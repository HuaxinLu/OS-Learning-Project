#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h" 
#include "sync.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"

void init_all(void)
{
	put_str("init all");
	idt_init();//初始化中断
	timer_init();//初始化定时器
	mem_init();
	thread_init();
	console_init();
	keyboard_init();  // 键盘初始化

	tss_init();
}
