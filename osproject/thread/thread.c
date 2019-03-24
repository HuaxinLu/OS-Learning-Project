#include "thread.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "list.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#define PG_SIZE 4096

struct task_struct* main_thread;//主线程pcb
struct list thread_ready_list;//就绪队列
struct list thread_all_list;//全部任务队列
static struct list_elem* thread_tag;//用于保存队列中的线程节点

extern void switch_to(struct task_struct* cur, struct task_struct* next);

//获取当前运行的pcb指针
struct task_struct* running_thread()
{
	uint32_t esp;
	asm("mov %%esp, %0":"=r" (esp));
	//取esp高20位（因为1个pcb占一页）
	return (struct task_struct*)(esp & 0xfffff000);
}

//由kernel_thread去执行function
static void kernel_thread(thread_func* function, void* func_arg)
{
	//执行前要开中断，避免时钟中断被屏蔽
	intr_enable();
	function(func_arg);
}

//初始化线程栈
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg)
{
	//先预留中断使用的栈空间
	pthread->self_kstack -= sizeof(struct intr_stack);
	//再预留线程栈空间
	pthread->self_kstack -= sizeof(struct thread_stack);	
	
	struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;
	kthread_stack->ebp = kthread_stack->ebx = kthread_stack->edi = kthread_stack->esi = 0;
}
//初始化线程基本信息
void init_thread(struct task_struct* pthread, char* name, int prio)
{
	memset(pthread, 0, sizeof(*pthread));
	strcpy(pthread->name, name);

	if(pthread == main_thread)
		pthread->status = TASK_RUNNING;
	else
		pthread->status = TASK_READY;
	
	pthread->priority = prio;
	//self_stack是线程自己在内核态下使用的栈顶地址
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->ticks = 0;
	pthread->elapsed_ticks = 0;
	pthread->pgdir = NULL;
	pthread->stack_magic = 0x12345678;//自定义的魔数
}

//创建线程
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg)
{
	struct task_struct* thread = get_kernel_pages(1);//控制块都存放在内核空间
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);

	//确保之前不在队列中
	ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
	ASSERT(!elem_find(&thread_all_list, &thread->general_tag));
	//加入线程队列
	list_append(&thread_ready_list, &thread->general_tag);
	list_append(&thread_all_list, &thread->all_list_tag);
	//asm volatile("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %%esi; ret": :"g" (thread->self_kstack) : "memory");
	
	return thread;
}

static void make_main_thread(void)
{
	//因为main线程已经运行，当时mov esp, 0xc009f000
	//就是为其预留PCB的，因此pcb地址位0xc009e000
	//不需要另外分配一页
	main_thread = running_thread();
	init_thread(main_thread, "main", 8);
	//添加main线程,main线程是当前运行的线程，只添加在alllist
	ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
	list_append(&thread_all_list, &main_thread->all_list_tag); 
}

void schedule()
{
	ASSERT(intr_get_status() == INTR_OFF);
	struct task_struct* cur = running_thread();
	if(cur->status == TASK_RUNNING)
	{
		//时间片到了
		ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
		list_append(&thread_ready_list, &cur->general_tag);
		//重置时间片
		cur->ticks = cur->priority;
		cur->status = TASK_READY;
	}
	else
	{
		//不是因为时间片到了
	}
	ASSERT(!list_empty(&thread_ready_list));
	thread_tag = NULL;//清空索引
	thread_tag = list_pop(&thread_ready_list);
	struct task_struct* next = elem2entry(struct task_struct, general_tag, thread_tag);
	next->status = TASK_RUNNING;
	switch_to(cur,next);

}

//当前线程将自己阻塞
void thread_block(enum task_status stat)
{
	//只有在以下三种状态才不会被调度
	ASSERT((stat == TASK_BLOCKED) || (stat == TASK_WAITING) || (stat == TASK_HANGING));
	enum intr_status old_status = intr_disable();
	struct task_struct* cur_thread = running_thread();
	cur_thread->status = stat;
	schedule();//当前线程换下处理器
	intr_set_status(old_status);
}
//解除阻塞
void thread_unblock(struct task_struct* pthread)
{
	enum intr_status old_status = intr_disable();
	ASSERT((pthread->status == TASK_BLOCKED) || (pthread->status == TASK_WAITING) || (pthread->status == TASK_HANGING));
	if(pthread->status != TASK_READY)
	{
		ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
		list_push(&thread_ready_list, &pthread->general_tag);//放在队列最前面，尽快得到调度
		pthread->status = TASK_READY;
	}
	intr_set_status(old_status);
}


void thread_init(void)
{
	put_str("thread_init start\n");
	list_init(&thread_ready_list);
	list_init(&thread_all_list);
	make_main_thread();
	put_str("thread_init end\n");
}


