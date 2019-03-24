[bits 32]
section .text
global switch_to
switch_to:
	;栈中此处是返回地址
	push esi
	push edi
	push ebx
	push ebp
	mov eax, [esp + 20];得到栈中参数cur,当前任务
	mov [eax], esp;保存栈顶指针esp，保存在self_kstack
	
	mov eax, [esp + 24];得到栈中参数next
	mov esp, [eax];记录0级栈指针，被换上cpu时用来恢复0级栈
				  ;0级栈中保存来进程和线程的所有信息
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
