#include "print.h"
#include "init.h"
#include "debug.h"
#include "string.h"
#include "memory.h"
#include "thread.h"
#include "sync.h"
#include "console.h"
#include "keyboard.h"
#include "interrupt.h"
#include "ioqueue.h"
void k_thread_a(void*);
void k_thread_b(void*);
int main(void)
{
	put_char('a');
	put_int(0x1234);
	put_str("hello world\n");
	init_all();
	thread_start("consumer_a", 31, k_thread_a, " a_");	
	thread_start("consumer_b", 31, k_thread_b, " b_");
 
	asm volatile("sti");//开中断
    //put_str("start os");	
	while(1)
	{
	//	console_put_str("Main");
	}

	return 0;
}


void k_thread_a(void* arg)
{
	while(1)
	{
		enum intr_status old_status = intr_disable();
		if (!ioq_empty(&kbd_buf)) 
		{
			console_put_str(arg);
			char byte = ioq_getchar(&kbd_buf);
			console_put_char(byte);
		}
    	intr_set_status(old_status);
	}
}

void k_thread_b(void* arg)
{
	while(1)
	{
		enum intr_status old_status = intr_disable();
		if (!ioq_empty(&kbd_buf)) 
		{
			console_put_str(arg);
	 		char byte = ioq_getchar(&kbd_buf);
	 		console_put_char(byte);
    	}
      	intr_set_status(old_status);
	}
}
