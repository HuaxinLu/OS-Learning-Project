#include "sync.h"
#include "global.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
//初始化信号量
void sema_init(struct semaphore * psema, uint8_t value)
{
	psema->value = value;
	list_init(&psema->waiters);//初始化等待队列
}
//初始化锁
void lock_init(struct lock * plock)
{
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_init(&plock->semaphore, 1);//信号量初值为1
}
//信号量down操作
void sema_down(struct semaphore * psema)
{
	//关中断，保证原子操作
	enum intr_status old_status = intr_disable();
	while(psema->value == 0)//表示已经被持有
	{
		ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));//确保当前线程不在队列中
	list_append(&psema->waiters, &running_thread()->general_tag);
	thread_block(TASK_BLOCKED);//阻塞自己
	}
	psema->value--;//恢复之后持有
	ASSERT(psema->value == 0);
	intr_set_status(old_status);
}
//信号量的up操作
void sema_up(struct semaphore* psema) 
{
	enum intr_status old_status = intr_disable();
	ASSERT(psema->value == 0);
	if (!list_empty(&psema->waiters)) //有等待的任务
	{ 
		struct task_struct* thread_blocked = elem2entry(struct task_struct, general_tag, \
		                                     list_pop(&psema->waiters));
		thread_unblock(thread_blocked);
	}
	psema->value++;
	ASSERT(psema->value == 1);
	intr_set_status(old_status);
}
//获取锁
void lock_acquire(struct lock* plock)
{
	if(plock->holder != running_thread())//排除自己已经持有锁但还未释放
	{
		//put_str("get lock\n");
		sema_down(&plock->semaphore);
		plock->holder = running_thread();
		ASSERT(plock->holder_repeat_nr == 0);
		plock->holder_repeat_nr = 1;
	}
	else
	{
		plock->holder_repeat_nr++;
	}
}
//释放锁
void lock_release(struct lock* plock) 
{
	ASSERT(plock->holder == running_thread());
	if (plock->holder_repeat_nr > 1) 
	{
		plock->holder_repeat_nr--;
		return;
	}
	ASSERT(plock->holder_repeat_nr == 1);
	plock->holder = NULL;
	plock->holder_repeat_nr = 0;
	sema_up(&plock->semaphore);	
}

