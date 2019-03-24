#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"
#include "print.h"
#define PIC_M_CTRL 0X20
#define PIC_M_DATA 0X21
#define PIC_S_CTRL 0XA0
#define PIC_S_DATA 0XA1

//目前支持的中断数 33个
#define IDT_DESC_CNT 0x30

#define EFLAGS_IF 0x00000200
#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl; popl %0" : "=g" (EFLAG_VAR))

//中断门描述结构体
struct gate_desc
{
	uint16_t func_offset_low_word;//中断函数偏移低字
	uint16_t selector;//中断目标代码段选择子
	uint8_t dcount;//固定值，不考虑
	uint8_t attribute;//属性值
	uint16_t func_offset_high_word;	
};

static struct gate_desc idt[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT];//声明kernel.S中的处理中断函数入口数组

char* intr_name[IDT_DESC_CNT];//用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT];//定义中断处理函数数组

//通用的中断处理函数，一般作为异常处理
static void general_intr_handler(uint8_t vec_nr)
{
	if(vec_nr == 0x27 || vec_nr == 0x2f)
		return;//IRQ7会产生伪中断，无需处理，0x2f是从片最后一个irq保留
	set_cursor(0);//重置光标为屏幕左上角
	int cursor_pos = 0;
	while(cursor_pos < 320) {
		put_char(' ');
		cursor_pos++;
	}//清3行
	set_cursor(0);

	put_str("!!!! exception message begin !!!!\n");
	set_cursor(88);//第二行第八个字符开始打印
	put_str(intr_name[vec_nr]);
	if (vec_nr == 14) { /* PG pagefault错误, 打印出来 */
		int page_fault_vaddr = 0;
		asm volatile("movl %%cr2, %0":"=r"(page_fault_vaddr));
		put_str("\npage fault addr is: "); 
		put_int(page_fault_vaddr);
	}
	
	put_str("\n!!!! exception message end !!!\n");
	while(1);
}
//完成一般中断处理函数注册和异常名称注册
static void exception_init()
{
	int i;
	for(i = 0; i < IDT_DESC_CNT; i++)
	{
		//先设定一般处理函数，测试用
		idt_table[i] = general_intr_handler;
		intr_name[i] = "unknown";
	}
	intr_name[0] = "#DE Divide Error"; 
	intr_name[1] = "#DB Debug Exception";
	intr_name[2] = "NMI Interrupt";
	intr_name[3] = "#BP Breakpoint Exception";
	intr_name[4] = "#OF Overflow Exception";
	intr_name[5] = "#BR Bound Range Exceed Exception";
	intr_name[6] = "#UD Invaild Opcode Exception";
	intr_name[7] = "#NM Device Not Available Exception";
	intr_name[8] = "#DF Double Fault Exception";
	intr_name[9] = "Coprocessor Segment Overrun";
	intr_name[10] = "#TS Invaild TSS Exception";
	intr_name[11] = "#NP Segment Not Present";
	intr_name[12] = "#SS Stack Fault Exception";
	intr_name[13] = "#GP General Protection Exception";
	intr_name[14] = "#PF Page-Fault Exception";
	// intr_name[15] intel 保留
	intr_name[16] = "#MF x87 FPU Floating-Point Error";
	intr_name[17] = "#AC Alignment Check Exception";
	intr_name[18] = "#MC Machine-Check Exception";
	intr_name[19] = "#XF SIMD Floating-Point Exception";
}



static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function)
{
	p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000ffff;
	p_gdesc->selector = SELECTOR_K_CODE;
	p_gdesc->dcount = 0;
	p_gdesc->attribute = attr;
	p_gdesc->func_offset_high_word = ((uint32_t)function & 0xffff0000) >> 16;
}

//获取中断状态
enum intr_status intr_get_status()
{
	uint32_t eflags = 0;
	GET_EFLAGS(eflags);
	return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

//开中断并返回中断前的状态
enum intr_status intr_enable()
{
	enum intr_status old_status;
	if(INTR_ON == intr_get_status())
	{
		old_status = INTR_ON;
	}
	else
	{
		asm volatile("sti");
		old_status = INTR_OFF;
	}
	return old_status;
}

//关中断并返回关中断前的状态
enum intr_status intr_disable()
{
	enum intr_status old_status;
	if(INTR_ON == intr_get_status())
	{
		old_status = INTR_ON;
		asm volatile("cli" : : : "memory" );//关中断，cli指令将if置0
	}
	else
	{
		old_status = INTR_OFF;
	}
	return old_status;
}

//设置中断状态
enum intr_status intr_set_status(enum intr_status status)
{
	return status == INTR_ON ? intr_enable() : intr_disable();
}


static void idt_desc_init()
{
	int i;
	for(i=0; i < IDT_DESC_CNT; i++)
	{
		make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
	}
	put_str("idt_desc_init_done\n");
}

//中断注册函数
void register_handler(uint8_t vector_no, intr_handler function)
{
	idt_table[vector_no] = function;
}

//初始化8259a
static void pic_init()
{
	//初始化主片
	outb(PIC_M_CTRL, 0x11);			//ICW1:边沿触发，级联，需要ICW4
	outb(PIC_M_DATA, 0x20);			//ICW2:起始中断向量号0x20
	outb(PIC_M_DATA, 0X04);			//ICW3:IR2接从片
	outb(PIC_M_DATA, 0X01);			//ICW4:8086模式，正常EOI
	//初始化从片
	outb(PIC_S_CTRL, 0x11);
	outb(PIC_S_DATA, 0X28);			//起始中断0x28,即IR8-15为0x28-2f
	outb(PIC_S_DATA, 0X02);			//设置从片到主片的IR2引脚
	outb(PIC_S_DATA, 0X01);			
	//打开主片上的IR0，只接受时钟中断
	outb(PIC_M_DATA, 0Xfe);
	outb(PIC_S_DATA, 0XFF);
	put_str("pic init done");
	//测试键盘，关闭其他中断
	outb(PIC_M_DATA, 0xfc);
	outb(PIC_S_DATA, 0xff);
}
uint64_t idt_operand = 0;
void idt_init()
{
	put_str("idt_init_start\n");
	idt_desc_init();//初始化中断描述表
	exception_init();//初始化异常处理函数和名称
	pic_init();//初始化8259a
	//加载idt
	idt_operand = ((sizeof(idt)-1) | ((uint64_t)((uint32_t)idt) << 16)) ;
	asm volatile("lidt %0" : : "m"(idt_operand));
	put_str("idt_init_finish\n");
}




