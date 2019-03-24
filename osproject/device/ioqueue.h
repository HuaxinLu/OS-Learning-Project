#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "stdint.h"
#include "thread.h"
#include "sync.h"

#define bufsize 64

//环形队列
struct ioqueue
{
	struct lock lock;
	struct task_struct* producer;//生产者，缓冲区不满就往里面放数据
	struct task_struct* consumer;//消费者，缓冲区不空就往里面读数据
	char buffer[bufsize];//缓冲区大小
	uint32_t head;//队首
	uint32_t tail;//队尾
};



void ioqueue_init(struct ioqueue* ioq);
bool ioq_full(struct ioqueue* ioq);
bool ioq_empty(struct ioqueue* ioq);
char ioq_getchar(struct ioqueue* ioq);
void ioq_putchar(struct ioqueue* ioq, char byte);

#endif
