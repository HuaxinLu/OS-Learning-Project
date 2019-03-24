	%include "boot.inc"
	section loader vstart=LOADER_BASE_ADDR
	;jmp loader_start

	GDT_BASE:			dd	0x00000000
						dd  0x00000000
	CODE_DESC:		    dd  0x0000FFFF
						dd  DESC_CODE_HIGH4
	DATA_STACK_DESC:	dd	0x0000FFFF
						dd  DESC_DATA_HIGH4
	VIDEO_DESC:         dd  0x80000007
						dd  DESC_VIDEO_HIGH4

	GDT_SIZE equ $-GDT_BASE
	GDT_LIMIT equ GDT_SIZE-1
	times 60 dq 0
	SELECTOR_CODE equ (0x0001<<3)+TI_GDT+RPL0
	SELECTOR_DATA equ (0x0002<<3)+TI_GDT+RPL0
	SELECTOR_VIDEO equ (0x0003<<3)+TI_GDT+RPL0
	
	total_mem_bytes dd 0

	gdt_ptr dw GDT_LIMIT
		    dd GDT_BASE
	;loadermsg db '2 loader in real' ;测试用代码
	
	ards_buf times 224 db 0	;ARDS存储空间
	ards_nr dw 0			;记录ARDS结构体数量

loader_start: 	;使用0x15中断子功能0xe820获取内存布局
	xor ebx, ebx			;第一次调用时清零
	mov edx, 0x534d4150		;字符串‘SMAP’
	mov di, ards_buf		;ards缓冲区
.e820_mem_get_loop:
	mov eax, 0x0000e820		;执行int 15后，eax变为‘0x534d4150’，需要重新赋值
	mov ecx, 20				;ards地址范围描述符结构大小位20字节
	int 0x15				;调用15号中断
	jc .e820_failed_so_try_e801	;cf位是1则发生错误
	add di, cx				;使di增加20个字节指向新的缓冲区位置
	inc word [ards_nr]		;记录ards的数量
	cmp ebx, 0				;如果ebx为0,说明全部返回
	jnz .e820_mem_get_loop
	;在所有的ards结构中 找出base_add_low+length_low 的最大值，即内存容量
	mov cx, [ards_nr]
	;遍历结构体，循环次数是ards的数量
	mov ebx, ards_buf
	xor edx, edx			;edx是最大内存容量，先清零
.find_max_mem_area:
	;无需判断type，最大内存块一定是可使用的
	mov eax, [ebx]			;基地址的低32位
	add eax, [ebx+8]		;长度的低32位
	add ebx, 20				;缓冲区指向下一个ards结构
	cmp edx, eax
;冒泡排序
	jge .next_ards
	mov edx, eax			;edx为总内存大小、
.next_ards:
	loop .find_max_mem_area
	jmp .mem_get_ok
	
;e801子功能获取内存大小
.e820_failed_so_try_e801:
	mov ax, 0xe801
	int 0x15
	jc .e801_failed_so_try_88
	;先算出低15MB的内存。单位为B
	mov cx, 0x400			;cx作为乘数 0x400=1024
	mul cx					;ax×cx
	shl edx, 16				;这里用的是16位操作数的乘法，高16位在edx寄存器，低16位在eax寄存器
	and eax, 0x0000FFFF		;
	add edx, 0x100000		;历史遗留
	mov esi, edx			;把低于15mb的内存备份
	;再算出16MB以上的内存，单位B
	xor eax, eax
	mov ax, bx
	mov ecx, 0x10000		;64k
	mul ecx					;32位操作数乘法，高32位存入edx，低32位在eax
	add esi, eax			;这种方法只能检测出4GB的内存内，因此低32位就够拉
	mov edx, esi			;edx是总内存大小
	jmp .mem_get_ok

;88方法,只能显示到63MB
.e801_failed_so_try_88:
	mov ah, 0x88
	int 0x15
	jc .error_hlt
	and eax, 0x0000FFFF
	mov cx, 0x400
	shl edx, 16
	or edx, eax
	add edx, 0x100000
	
.mem_get_ok:
	mov [total_mem_bytes], edx
	jmp p_mode_start

.error_hlt:
	jmp $



;进入保护模式
p_mode_start:
	in al, 0x92
	or al, 00000010B
	out 0x92, al

	lgdt [gdt_ptr]
	mov eax, cr0
	or eax, 0x00000001
	mov cr0, eax

	jmp dword SELECTOR_CODE: p_mode_test

[bits 32]
p_mode_test:
	mov ax,SELECTOR_DATA
	mov ds,ax
	mov es,ax
	mov ss,ax
	mov esp,LOADER_STACK_TOP
	mov ax,SELECTOR_VIDEO
	mov gs,ax

	mov byte [gs:160], 'P'

;加载内核
	mov eax, KERNEL_START_SECTOR		;内核所在的扇区号
	mov ebx, KERNEL_BIN_BASE_ADDR		;从磁盘读出后写入的地址
	mov cx, 200		;读入的扇区数
	call rd_disk_m_32

	call setup_page	;初始化页表

	;将描述符表地址和偏移量写入内存gdt_ptr，一会用新地址重新加载
	sgdt [gdt_ptr]
	;将原来的vedio段描述符的基地址+0xC0000000,使其成为内核所在的高低址
	mov ebx, [gdt_ptr+2]
	or dword [ebx+0x18+4], 0xc0000000	;是第三个描述符，每个占8字节
	add dword [gdt_ptr+2], 0xc0000000
	add esp, 0xc0000000		;栈指针映射到内核地址
	;目录地址赋给cr3
	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax
	
	;打开cr0的pg位
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax
	;开启分页后，重新加载GDT
	lgdt [gdt_ptr]
	;强制刷新流水线，不刷也可以
	jmp SELECTOR_CODE:enter_kernel
	
enter_kernel:
	mov byte [gs:162], 'V'
	call kernel_init
	mov esp, 0xc009f000
	jmp KERNEL_ENTRY_POINT



;初始化内核
kernel_init:
	xor eax, eax
	xor ebx, ebx		;记录程序头表地址
	xor ecx, ecx		;记录程序头表中headers的数量
	xor edx, edx		;记录headers的尺寸
	
	
	mov dx, [KERNEL_BIN_BASE_ADDR+42]	;表示header的大小
	mov ebx, [KERNEL_BIN_BASE_ADDR+28]	;表示程序头表的地址
	add ebx, KERNEL_BIN_BASE_ADDR
	mov cx, [KERNEL_BIN_BASE_ADDR+44]
.each_segment:
	cmp byte [ebx+0], PT_NULL			;若位PTNULL，说明该headers未使用
	je .PTNULL
	push dword [ebx+16]
	mov eax, [ebx+4]
	add eax, KERNEL_BIN_BASE_ADDR		;被加载到的物理地址
	push eax							;源地址
	push dword [ebx+8]					;目的地址
	call mem_cpy						;完成段复制
	add esp, 12							;清理参数
.PTNULL:
	add ebx, edx						;edx为header的大小
	loop .each_segment
	ret

mem_cpy:
	cld
	push ebp
	mov ebp, esp
	push ecx							;先备份
	mov edi, [ebp+8]					;dst
	mov esi, [ebp+12]					;src
	mov ecx, [ebp+16]
	rep movsb							;逐字节拷贝
	;恢复环境
	pop ecx
	pop ebp
	ret



;创建页目录和页表
setup_page:
;先把页目录占用的空间清零
	mov ecx, 4096
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS+esi], 0
	inc esi
	loop .clear_page_dir

;开始创建目录表项PDE
.create_pde:
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x1000			;eax为第一个页表的位置
	mov ebx, eax			;为creatpte做准备
	;将页目录项0和0xc00都存为第一个页表的地址
	;这样0xc03fffff以下的地址和0x003fffff的地址都指向相同的页表
	or eax, PG_US_U | PG_RW_W | PG_P	;RW=1 p=1 US=1 表示用户属性，所有的特权级别都可以访问
	mov [PAGE_DIR_TABLE_POS+0x00], eax	;写入第一个页表的属性
	mov [PAGE_DIR_TABLE_POS+0xc00], eax;第3072个页表目录项也指向0x101000处
	;0xc00表示第768个页表的位置，这样设置表示768-1024的空间属于内核，下面的属于用户
	sub eax, 0x1000
	mov [PAGE_DIR_TABLE_POS+4092], eax	;目录的最后一项指向目录表自己的地址

;创建页表项
	mov ecx, 256			;1M低端内存/每页大小4K
	mov esi, 0
	mov edx, PG_US_U | PG_RW_W | PG_P	;用户属性
.creat_pte:
	mov [ebx+esi*4], edx	;ebx已经赋值过，为第一个页表的地址
	add edx, 4096
	inc esi
	loop .creat_pte

;创建内核其他页表的PDE
	mov eax, PAGE_DIR_TABLE_POS
	add eax, 0x2000			;此时eax为第二个页表的位置
	or eax, PG_US_U | PG_RW_W | PG_P
	mov ebx, PAGE_DIR_TABLE_POS
	mov ecx, 254			;范围为从769-1022的所有目录项数量
	mov esi, 769

.create_kernel_pde:
	mov [ebx+esi*4], eax
	inc esi
	add eax, 0x1000
	loop .create_kernel_pde
	ret



rd_disk_m_32:
	mov esi, eax                 ;备份扇区号
 	mov di, cx                   ;备份待读取的扇区数量
	mov dx, 0x1f2                ;端口号
	mov al, cl                   ;读取的扇区数量
	out dx, al                   ;输出    
	
	mov eax, esi                 ;恢复eax                    
	mov dx, 0x1f3                ;写入LBA低地址	
    out dx, al                   ;输出

    mov cl, 8                    ;
    shr eax, cl                  ;逻辑右移8位
    mov dx, 0x1f4                ;写入中地址
    out dx, al
  
    shr eax, cl                  ;继续右移8位
    mov dx, 0x1f5
    out dx, al

	shr eax, cl
    and al, 0x0f                 ;取24-27位
    or al, 0xe0                  ;设置7-4位为1,lba模式
    mov dx, 0x1f6
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20                 ;写入读命令
    out dx, al

.not_ready:
    nop
    in al, dx
    and al, 0x88                 ;控制器忙？控制器准备好数据？
    cmp al, 0x08
    jnz .not_ready               ;没准备好继续等待

    mov ax, di
    mov dx, 0x100
    mul dx                      ;一个扇区512字节每次读一个字
    mov cx, ax

    mov dx, 0x1f0

.go_on_read:
    in ax,dx
    mov [ebx],ax                 ;注意bx不能超过64k，否则无法寻址到
    add ebx,2
    loop .go_on_read
    ret



