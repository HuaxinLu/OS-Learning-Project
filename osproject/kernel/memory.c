#include "memory.h"
#include "print.h"
#include "stdint.h"
#include "debug.h"
#include "string.h"
#include "bitmap.h"
#include "sync.h"
#define PG_SIZE 4096
/*********************
0xc009f000是内核主线程栈顶，0xc009e000是内核主线程的pcb
一个页框大小的位图可表示128MB内存，位图位置安排在0xc009a000
这样本系统最大支持4个页框的位图，即512MB
*********************/
#define MEM_BITMAP_BASE 0xc009a000
#define K_HEAP_START 0xc0100000
//跨过1MB内存，使虚拟地址在逻辑上连续

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

/* 内存池结构,生成两个实例用于管理内核内存池和用户内存池 */
struct pool 
{
	struct bitmap pool_bitmap;	 // 本内存池用到的位图结构,用于管理物理内存
	uint32_t phy_addr_start;	 // 本内存池所管理物理内存的起始地址
	uint32_t pool_size;		 // 本内存池字节容量
	struct lock lock;		 // 申请内存时互斥
} kernel_pool, user_pool;


//在pf表示的虚拟内存池中申请pg_cnt个虚拟页，成功返回虚拟页的地址，失败返回NULL
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
	int vaddr_start = 0;
	int bit_idx_start = -1;
	uint32_t cnt = 0;
	if(pf == PF_KERNEL)//是在内核内存池
	{
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
		{
			put_str("vaddr_get failed\n");
			return NULL;
		}
		while(cnt < pg_cnt)
		{
			bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
		//put_str("vaddr_get successful\n");
	}
	else
	{
		//用户内存池，将来补充
		struct task_struct* cur = running_thread();
		bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
			return NULL;
		while(cnt < pg_cnt)
		{
			bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
		}
		vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
		ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
	}
	return (void*)vaddr_start;
}

//解析虚拟地址
//得到虚拟地址vaddr对应的pte指针
uint32_t* pte_ptr(uint32_t vaddr)
{
	//先访问到页表自己+用页目录项pde作为pte的索引访问到页表+用pte的索引访问到偏移
	uint32_t* pte = (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
	return pte;
}
//得到虚拟地址vaddr对应的pde指针
uint32_t* pde_ptr(uint32_t vaddr)
{
	uint32_t* pde = (uint32_t*)(0xfffff000 + PDE_IDX(vaddr) * 4);
	return pde;
}
//在m_pool指向的物理内存内池中分配一个物理页
static void* palloc(struct pool* m_pool)
{
	//扫描或设置位图要保证原子操作
	int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);//找一个物理页面
	if(bit_idx == -1)
	{
		put_str("palloc failed\n");
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
	uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
	return (void*)page_phyaddr;
}

//页表中添加虚拟地址到物理地址的映射
static void page_table_add(void* _vaddr, void* _page_phyaddr)
{
	ASSERT(_vaddr != NULL);
	ASSERT(_page_phyaddr != NULL);
	uint32_t vaddr = (uint32_t)_vaddr;
	uint32_t page_phyaddr = (uint32_t)_page_phyaddr;
	uint32_t* pde = pde_ptr(vaddr);
	uint32_t* pte = pte_ptr(vaddr);
	//执行*pte会访问到空的pde，所以必须保证pde创建完后才能指向*pte，会引发pagefault
	//首先判断该目录项是否已经存在
	if(*pde &  0x01)//判断P位，如果是1,已经存在
	{
		ASSERT(!(*pte & 0x01));//判断页表项是否存在
		//只要是创建页表，pte就不应该存在
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
	else
	{
		//页目录项不存在，首先要创造页目录项
		//页表中用到的页框一律从内核中分配
		uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
		*pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
		//分配到的物理页要清零，防止旧数据变为页表项
		//访问pde对应的物理地址，高20位取1即可，pte就是偏移
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		ASSERT(!(*pte & 0x01));
		*pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
	}
}

//分配pg_cnt个页空间
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
	ASSERT(pg_cnt > 0 && pg_cnt < 3840);	
/*
1 通过vaddr_get在虚拟内存池中申请虚拟地址
2 通过palloc在物理内存中申请物理页
3 完成映射
*/
	void* addr_start = vaddr_get(pf, pg_cnt);
	uint32_t vaddr = (uint32_t)addr_start;
	uint32_t cnt = pg_cnt;
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
	//虚拟地址连续，物理地址不连续，因此要做逐个映射
	while(cnt-- >0)
	{
		void* page_phyaddr = palloc(mem_pool);
		if(page_phyaddr == NULL)//失败要将已经申请的虚拟地址和物理页全部回滚
		{
			return NULL;
		}
		page_table_add((void*)vaddr, page_phyaddr);//在页表中做映射
		vaddr += PG_SIZE;
	}
	return addr_start;
}

//申请n页内存，成功返回物理地址，失败返回null
void* get_kernel_pages(uint32_t pg_cnt) 
{
	//put_str("get_kernel_pages start");
	//put_int(pg_cnt);
	void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if (vaddr != NULL) 
	{
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	}
	else
	{
		put_str("get_kernel_pages failed");
	}
	return vaddr;
}

//在用户空间中申请4k内存
void* get_user_pages(uint32_t pg_cnt)
{
	lock_acquire(&user_pool.lock);
	void* vaddr = malloc_page(PF_USER, pg_cnt);
	if (vaddr != NULL)
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	else
		put_str("get_user_pages failed");
	lock_release(&user_pool.lock);
	return vaddr;
}

//将地址vaddr与pf池中的物理地址相关联，仅支持一页空间分配
void* get_a_page(enum pool_flags pf, uint32_t vaddr)
{
	struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
	lock_acquire(&mem_pool->lock);
	//先将虚拟地址对应的位图置
	struct task_struct* cur = running_thread();
	int32_t bit_idx = -1;
	//若是用户进程申请用户内存，就修改用户进程的虚拟位图
	if(cur->pgdir != NULL && pf == PF_USER)
	{
		bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);
	}	
	else if(cur->pgdir == NULL && pf == PF_KERNEL)
	{
		bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
		ASSERT(bit_idx > 0);
		bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
	}
	else
	{
		PANIC("get_a_page: not allow kernel alloc userpages or user alloc kernelpages");
	}
	void* page_phyaddr = palloc(mem_pool);
	if(page_phyaddr == NULL)
		return NULL;
	page_table_add((void*)vaddr, page_phyaddr);
	lock_release(&mem_pool->lock);
	return (void*)vaddr;
}
//得到虚拟地址对应的物理地址
uint32_t addr_v2p(uint32_t vaddr)
{
	uint32_t* pte = pte_ptr(vaddr);
	//*pte是页表所在物理页框地址
	//去掉低12位的页表项属性+虚拟地址vaddr的低12位
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

//初始化内存池
static void mem_pool_init(uint32_t all_mem)
{
	put_str("mem_pool_init start");
	uint32_t page_table_size = PG_SIZE * 256;
/*
为什么是256：内核使用的内存包括第0页和第768个页目录指向的
页表+769-1022个页目录项指向的254个页表，共256个页框，
最后1023页目录项指向的是页目录表，不用重复计算
*/
	uint32_t used_mem = page_table_size + 0x100000;
/*
还需要加上实模式过渡的1MB内存
*/
	uint32_t free_mem = all_mem - used_mem;
	uint32_t all_free_pages = free_mem / PG_SIZE;
	uint16_t kernel_free_pages = all_free_pages / 2;
	uint16_t user_free_pages = all_free_pages - kernel_free_pages;
	//简化位图操作，余数不做处理，会丢内存
	//kernel bitmap
	uint32_t kbm_length = kernel_free_pages / 8;
	//user bitmap
	uint32_t ubm_length = user_free_pages / 8;
	//内核内存池的起始地址
	uint32_t kp_start = used_mem;
	//用户内存池的起始地址
	uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;
	
	kernel_pool.phy_addr_start = kp_start;
	user_pool.phy_addr_start = up_start;
	
	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;
	
	kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
	user_pool.pool_bitmap.btmp_bytes_len = ubm_length;
/*
位图是全局数据，长度不固定，因此指定一块内存来生成位图
*/
	//内核使用的最高地址是0xc009f000,这是主线程的栈地址
	//内核内存池的位图先在0xc009a000处
	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	//用户内存池的位图在内核之后
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);
	/* 输出内存池信息 */
	put_str("    kernel_pool_bitmap_start: ");
	put_int((uint32_t)(kernel_pool.pool_bitmap.bits));
	put_str(" kernel_pool_phy_addr_start: ");
	put_int(kernel_pool.phy_addr_start);
	put_char('\n');
	put_str("    user_pool_bitmap_start: ");
	put_int((uint32_t)(user_pool.pool_bitmap.bits));
	put_str(" user_pool_phy_addr_start: ");
	put_int(user_pool.phy_addr_start);
	put_char('\n');
	/* 将位图置0 */
	//ASSERT(kernel_pool.pool_bitmap.bits != NULL);
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);
	//初始化锁
	lock_init(&kernel_pool.lock);
	lock_init(&user_pool.lock);	


	//开始初始化虚拟地址的位图
	kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
	//位图的数组指向一块未使用的内存，目前定位在内核内存池和用户内存池之外
	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length + ubm_length);
	kernel_vaddr.vaddr_start = K_HEAP_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);
	put_str("  mem_pool_init done\n");	
}

void mem_init(void) {
	put_str("mem_init start.\n");
	uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
	mem_pool_init(mem_bytes_total);
	put_str("mem_init done.\n");
}
