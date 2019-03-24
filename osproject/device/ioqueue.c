#include "ioqueue.h"
#include "interrupt.h"
#include "global.h"
#include "debug.h"

//初始化队列
void ioqueue_init(struct ioqueue* ioq)
{
	lock_init(&ioq->lock);
	ioq->producer = NULL;
    ioq->consumer = NULL;
	ioq->head = 0;
	ioq->tail = 0;
}

//返回缓冲区的下一个位置
static uint32_t next_pos(uint32_t pos)
{
	return (pos+1)%bufsize;
}
//判断队列是否已满
bool ioq_full(struct ioqueue* ioq)
{
	ASSERT(intr_get_status() == INTR_OFF);
	return next_pos(ioq->tail) == ioq->head;
}
//判断队列是否已空
bool ioq_empty(struct ioqueue* ioq)
{
	ASSERT(intr_get_status() == INTR_OFF);
	return ioq->head == ioq->tail;
}
//使当前消费者等待
static void ioq_wait(struct task_struct** waiter)
{
	ASSERT(waiter!=NULL && *waiter!=NULL);
	*waiter = running_thread();
	thread_block(TASK_BLOCKED);
}
//唤醒消费者
static void wakeup(struct task_struct** waiter)
{
	ASSERT(waiter!=NULL && *waiter!=NULL);
	thread_unblock(*waiter);
	*waiter = NULL;
}
//消费者从队列中获取一个char
char ioq_getchar(struct ioqueue* ioq)
{
	ASSERT(intr_get_status() == INTR_OFF);
	//如果缓冲区为空
	while(ioq_empty(ioq))
	{
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->consumer);
		lock_release(&ioq->lock);
	}
	//如果不为空
	char byte = ioq->buffer[ioq->tail];
	ioq->tail = next_pos(ioq->tail);
	
	//唤醒生产者
	if(ioq->producer != NULL)
	{
		wakeup(&ioq->producer);
	}
	return byte;
}
//生产者写一个byte
void ioq_putchar(struct ioqueue* ioq, char byte)
{
	ASSERT(intr_get_status() == INTR_OFF);
	//如果缓存满了，把生产者记为自己
	while(ioq_full(ioq))
	{
		lock_acquire(&ioq->lock);
		ioq_wait(&ioq->producer);
		lock_release(&ioq->lock);
	}
	ioq->buffer[ioq->head] = byte;
	ioq->head=next_pos(ioq->head);
	if(ioq->consumer != NULL)
	{
		wakeup(&ioq->consumer);
	}
}
