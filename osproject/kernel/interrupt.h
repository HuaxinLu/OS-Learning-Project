#ifndef __KERNEL_INTERRUPT_H
#define __KERNEL_INTERRUPT_H

#include "stdint.h"

typedef void* intr_handler;

enum intr_status
{
	INTR_ON,
	INTR_OFF
};

void idt_init();
enum intr_status intr_get_status();
enum intr_status intr_disable();
enum intr_status intr_enable();
enum intr_status intr_set_status(enum intr_status status);
void register_handler(uint8_t vector_no, intr_handler function);
#endif
