#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
//虚拟地址池
struct virtual_addr
{
	struct bitmap vaddr_bitmap;//虚拟地址使用的位图结构
	uint32_t vaddr_start;//起始地址
};//实例，用来给内核分配虚拟地址

struct virtual_addr kernel_vaddr;


//内存池标记，判断用哪个内存池
enum pool_flags
{
	PF_KERNEL = 1,//内核内存池
	PF_USER = 2//用户内存池
};

#define	PG_P_1 	1  	/* 页表项和页目录项存在标志位 */
#define PG_P_0	0	/**/
#define PG_RW_R	0	/* RW可读	*/
#define PG_RW_W	2	/* RW可写 */
#define PG_US_S 0	/* U/S标志 系统级 */
#define PG_US_U	4	/* U/S标志 用户级 */

extern struct pool kernel_pool;
extern struct pool user_pool;

void mem_init(void);
void* get_kernel_pages(uint32_t pg_cnt);
#endif

