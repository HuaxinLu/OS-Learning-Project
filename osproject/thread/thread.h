#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"
//自定义通用函数类型，在线程函数中作为形参类型
typedef void thread_func(void*);

//进程和线程的状态
enum task_status
{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WAITING,
	TASK_HANGING,
	TASK_DIDE
};

//中断栈，用于中断发生时保护上下文环境
struct intr_stack
{
	uint32_t vec_no;//压入的中断号
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;		/*无用,pushad*/
	uint32_t ebx;			
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	/*通用寄存器*/
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	/*cpu从低特权级到高特权级时候压入*/
	uint32_t err_code;
	void (*eip)(void);
	uint32_t cs;
	uint32_t eflags;
	void *esp;
	uint32_t ss;
};

//线程栈，用于存储线程中待执行的函数
//此结构在自己内核栈中位置不固定
struct thread_stack
{
	//ABI规范的要求，以下四个寄存器+esp归主调函数
	//被调函数必须保护号这几个寄存器
	//esp的值会由调用约定来保证
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	//线程第一次执行时，eip指向待调用的函数
	//其他时候，指向swith to的返回地址
	void (*eip)(thread_func* func, void* func_arg);
	
	//以下仅供第一次调度使用
	void (*unused_retaddr);//占位
	thread_func* function;//kernel_thread所调用的函数名
	void* func_arg;//调用函数所需要的参数
};

//程序控制块PCB
struct task_struct
{
	uint32_t* self_kstack;//内核栈
	enum task_status status;
	uint8_t priority;
	char name[16];
	uint8_t ticks;//每次在处理器上执行的滴答数
	uint32_t elapsed_ticks;//此任务总共运行了多久
	struct list_elem general_tag;
	struct list_elem all_list_tag;//线程队列中的节点
	uint32_t* pgdir;//进程自己页表的虚拟地址
	struct virtual_addr userprog_vaddr;//用户进程的虚拟地址
	uint32_t stack_magic;//边界标记，检测溢出
};
struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg);
struct task_struct* running_thread();
void schedule();
void thread_init(void);

void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);



#endif
