#ifndef __LIB_IO_H
#define __LIB_IO_H
#include "stdint.h"

//向端口中写入一个字节
static inline void outb(uint16_t port, uint8_t data)
{
	//b0表示使用al w1表示使用dx 
	asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}
//将addr处的word_cnt个字写入端口port
static inline void outsw(uint16_t port, const void* addr, uint32_t word_cnt)
{
	//+表示此限制即做输入又做输出
	//outsw表示把ds:esi的16位内容写入port端口
	asm volatile ("cld; rep outsw" : "+S" (addr), "+c" (word_cnt) : "d" (port));
}
//读取端口
static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	//=a表示只写
	//Nd表示0-255的立即数传给dx
	asm volatile("inb %w1, %b0" : "=a" (data) : "Nd" (port));
	return data;
}
//从端口中读取n个字写入addr
static inline void insw(uint16_t port, void* addr, uint32_t word_cnt)
{
	//写入es:edi
	asm volatile("cld; rep insw" : "+D" (addr), "+c" (word_cnt) : "d" (port) : "memory");
}

#endif
