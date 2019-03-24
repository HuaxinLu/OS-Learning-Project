#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

void panic_spin(char* filename, int line, const char* func, const char* condition);
//...表示其参数可变 __VA_ARGS__代表与...相对应的参数
#define PANIC(...) panic_spin (__FILE__, __LINE__, __func__, __VA_ARGS__)
#ifdef NODEBUG
	#define ASSERT(CONDITION) (void(0))
#else
	#define ASSERT(CONDITION) if(CONDITION){} else {PANIC(#CONDITION);}
	//#符号是将参数转化位字符串
#endif
#endif
