[bits 32]
%define ERROR_CODE			;若已经压入错误码，保持格式统一
%define ZERO push 0			;若没有压入错误码，手工压入0

extern put_str				;声明外部函数
extern idt_table			;中断处理函数 C语言的
section .data
intr_str db "interrupt occur!", 0x0a, 0
global intr_entry_table
intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:				;每一个中断处理程序都要压入中断向量号
							;一个中断类型一个程序，自己知道自己向量号
	%2
	;保存现场
	push ds
	push es
	push fs
	push gs
	pushad

	;如果是从片上进入中断，要往主片和从片发送EOI
	mov al, 0x20			;中断结束命令EOI
	out 0xa0, al			;主片端口
	out 0x20, al			;从片端口
	
	push %1					;不管中断处理函数是否需要参数，都一律压入中断号
	call [idt_table + %1 * 4]	;调用idt_table中的C版本中断处理函数
	jmp intr_exit			;调用完成

section .data
	dd intr%1entry			;储存进入中断的入口程序的地址
%endmacro

section .text
global intr_exit
intr_exit:
	;恢复上下文环境
	add esp, 4				;跳过中断号
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 4				;跳过错误码
	iretd


VECTOR 0X00, ZERO
VECTOR 0X01, ZERO
VECTOR 0X02, ZERO
VECTOR 0X03, ZERO
VECTOR 0X04, ZERO
VECTOR 0X05, ZERO
VECTOR 0X06, ZERO
VECTOR 0X07, ZERO
VECTOR 0X08, ZERO
VECTOR 0X09, ZERO
VECTOR 0X0a, ZERO
VECTOR 0X0b, ZERO
VECTOR 0X0c, ZERO
VECTOR 0X0d, ZERO
VECTOR 0X0e, ZERO
VECTOR 0X0f, ZERO
VECTOR 0X10, ZERO
VECTOR 0X11, ZERO
VECTOR 0X12, ZERO
VECTOR 0X13, ZERO
VECTOR 0X14, ZERO
VECTOR 0X15, ZERO
VECTOR 0X16, ZERO
VECTOR 0X17, ZERO
VECTOR 0X18, ZERO
VECTOR 0X19, ZERO
VECTOR 0X1a, ZERO
VECTOR 0X1b, ZERO
VECTOR 0X1c, ZERO
VECTOR 0X1d, ZERO
VECTOR 0X1e, ERROR_CODE
VECTOR 0X1f, ZERO
VECTOR 0X20, ZERO
VECTOR 0x21,ZERO	;键盘中断对应的入口
VECTOR 0x22,ZERO	;级联用的
VECTOR 0x23,ZERO	;串口2对应的入口
VECTOR 0x24,ZERO	;串口1对应的入口
VECTOR 0x25,ZERO	;并口2对应的入口
VECTOR 0x26,ZERO	;软盘对应的入口
VECTOR 0x27,ZERO	;并口1对应的入口
VECTOR 0x28,ZERO	;实时时钟对应的入口
VECTOR 0x29,ZERO	;重定向
VECTOR 0x2a,ZERO	;保留
VECTOR 0x2b,ZERO	;保留
VECTOR 0x2c,ZERO	;ps/2鼠标
VECTOR 0x2d,ZERO	;fpu浮点单元异常
VECTOR 0x2e,ZERO	;硬盘
VECTOR 0x2f,ZERO	;保留
