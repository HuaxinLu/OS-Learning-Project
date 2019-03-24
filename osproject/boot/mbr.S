%include "boot.inc"
SECTION MBR vstart=0x7c00
	mov ax,cs
	mov ds,ax
    mov es,ax
	mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800 				;gs是全局段寄存器，b8000是显存地址
    mov gs,ax

	mov ax,0600h				;清屏
	mov bx,0700h
	mov cx,0
	mov dx,184fh
	int 10h

	mov byte [gs:0x00],'M'
    mov byte [gs:0x01],0xa4
	mov byte [gs:0x02],'B'
	mov byte [gs:0x03],0xa4
	mov byte [gs:0x04],'R'
	mov byte [gs:0x05],0Xa4

	mov eax,LOADER_START_SECTOR	;起始扇区的线性地址
	mov bx,	LOADER_BASE_ADDR	;需要被写入的地址
	mov cx,4					;待读取的扇区数量
	call rd_disk_m_16			;调用读取函数

    jmp LOADER_BASE_ADDR+0x300			;跳到被写入的地址开始运行

	
rd_disk_m_16:

	mov esi,eax					;备份扇区号
    mov di,cx					;备份待读取的扇区数量

	mov dx,0x1f2				;端口号
	mov al,cl					;读取的扇区数量
	out dx,al					;输出    
	mov eax,esi					;恢复eax					

	mov dx,0x1f3				;写入LBA低地址
	out dx,al					;输出

	mov cl,8					;
	shr eax,cl					;逻辑右移8位
	mov dx,0x1f4				;写入中地址
	out dx,al

	shr eax,cl					;继续右移8位
	mov dx,0x1f5
	out dx,al

	shr eax,cl					
	and al,0x0f					;取24-27位
	or al,0xe0					;设置7-4位为1,lba模式
	mov dx,0x1f6
	out dx,al

	mov dx,0x1f7
	mov al,0x20					;写入读命令
    out dx,al

.not_ready:
	nop
	in al,dx
	and al,0x88					;控制器忙？控制器准备好数据？
	cmp al,0x08
	jnz .not_ready				;没准备好继续等待

	mov ax,di
	mov dx,256
	mul dx						;一个扇区512字节每次读一个字
	mov cx,ax

	mov dx,0x1f0

.go_on_read:
	in ax,dx
	mov [bx],ax					;注意bx不能超过64k，否则无法寻址到
	add bx,2
	loop .go_on_read
	ret

	times 510-($-$$) db 0
	db 0x55,0xaa
