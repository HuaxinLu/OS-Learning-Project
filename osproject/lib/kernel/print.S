TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0

[bits 32]
section .text

;功能描述：设置光标位置
global set_cursor
set_cursor:
	push eax
	push ebx
	push edx
	mov ebx, [esp+16]
.set_cursor_begin:
	mov dx, 0x03d4
	mov al, 0x0e		; 光标高8位
	out dx, al 
	mov dx, 0x03d5
	mov al, bh
	out dx, al 

	mov dx, 0x03d4
	mov al, 0x0f		; 光标低8位
	out dx, al 
	mov dx, 0x03d5
	mov al, bl
	out dx, al
.set_cursor_done:
	pop edx
	pop ebx
	pop eax
ret

;功能描述：打印字符串（以0结尾）
global put_str
put_str:
	;首先把需要用的寄存器入栈
	push ebx			;ebx来寻址
	push ecx			;储存参数
	xor ecx, ecx
	mov ebx, [esp+12]	;两个32位的参数+返回地址
	
.goon:
	mov cl, [ebx]
	cmp cl, 0			;判断是否打完
	jz .str_over
	push ecx
	call put_char
	
	add esp, 4			;回收栈空间
	inc ebx
	jmp .goon
.str_over:
	pop ecx
	pop ebx
	ret

;功能描述：打印数字 32位
global put_int
put_int:
	push ecx
	push ebp
	push edx
	mov ecx, 4			;总共打4次，dx是偏移
	mov ebp, esp		;ebp用于栈内寻址
.put_int_loop:
	mov eax, [ebp+15+ecx]
	mov edx, eax
	shr edx, 4
	and edx, 0xf
	call print_a_number
	mov edx, eax
	and edx, 0xf
	call print_a_number
	loop .put_int_loop
	
	pop edx
	pop ebp
	pop ecx
	ret

print_a_number:
.16based_4bits:
	cmp edx, 9			;0-9和a-f分开处理
	jg .is_A2F
	add edx, '0'
	jmp .print_int
.is_A2F:
	add edx, 'a'
	sub edx, 10
.print_int:
	push edx
	call put_char
	add esp, 4
	ret

;功能描述：把栈中的第1个字符写入光标所在处
global put_char
put_char:
	pushad				;指令压入所有双子长的寄存器
	;需要保证gs中为正确的视频段选择子
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	;获取当前光标位置
	;先获得高8位
	mov dx, 0x3d4		;索引寄存器
	mov al, 0x0e		;提供光标位置的高8位
	out dx, al
	mov dx, 0x03d5		;通过3d5端口来获得或设置光标位置
	in al, dx
	mov ah, al
	;再获取低8位
	mov dx, 0x3d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx
	
	;将光标存入bx
	mov bx, ax
	;在栈中获取待打印的字符
	mov ecx, [esp+36]	;pushad压入4×8=32个字节，主调函数4字节的返回地址
	cmp cl, 0xd			;判断是不是回车
	jz .is_carriage_return
	cmp cl, 0xa			;判断是不是换行
	jz .is_line_feed
	cmp cl, 0x8			;判断是不是退格
	jz .is_backspace
	jmp .put_other


.is_backspace:
	dec bx
	shl bx, 1			;光标左移1位 ×2,得到的是对应的删除掉字符在显卡缓冲区的位置
	mov byte [gs:bx], 0x20	;待删除的字节补0
	inc bx				
	mov byte [gs:bx], 0x07	;字符属性
	shr bx, 1			;恢复老的光标
	jmp .set_cursor

.put_other:
	shl bx, 1
	mov [gs:bx], cl	
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	inc bx				;光标+1				
	cmp bx, 2000
	jl .set_cursor		;如果超过2000,去设置新的光标

.is_line_feed:
.is_carriage_return:
	xor dx, dx			;dx是被除数的高16位
	mov ax, bx			;ax是被除数的低16位
	mov si, 80
	div si				
	sub bx, dx			;取整

.is_carriage_return_end:
	add bx, 80
	cmp bx, 2000
.is_line_feed_end:
	jl .set_cursor

;屏幕行范围是0-24,滚屏原理是把1-24行搬运到0-23行，再将第24行空格填充
.roll_screen:
	cld
	mov ecx, 960
	mov esi, 0xc00b80a0	;第1行的行首
	mov edi, 0xc00b8000	;第0行的行首
	rep movsd
;将最后一行填充为空白
	mov ebx, 3840		;最后一行首字符的第一个字节偏移 1920×2
	mov ecx, 80			;一行80个字符

.cls:
	mov word [gs:ebx], 0x720	;黑底白字的空格
	add ebx, 2
	loop .cls
	mov bx, 1920		;光标重置为最后一行的首字符
.set_cursor:
;将光标设置为bx值
	;先设置为高8位
	mov dx, 0x03d4		;索引寄存器
	mov al, 0x0e		;光标位置的高八位
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al
	;再设置低八位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al
.put_char_done:
	popad
	ret
