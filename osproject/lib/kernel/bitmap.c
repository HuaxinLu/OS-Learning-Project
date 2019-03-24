#include "bitmap.h"
#include "string.h"
#include "debug.h"
#include "print.h"
void bitmap_init(struct bitmap* btmp)
{
	ASSERT(btmp != NULL);
	memset(btmp->bits, 0, btmp->btmp_bytes_len);
}

bool bitmap_scan_test(struct bitmap* btmp, uint32_t bit_idx)
{
	uint32_t byte_idx = bit_idx / 8;
	uint32_t bit_offset = bit_idx % 8;
	return (btmp->bits[byte_idx]) & (1<<bit_offset);
}

//在位图中连续申请cnt个位，成功返回起始位下标，失败返回-1
int bitmap_scan(struct bitmap* btmp, uint32_t cnt)
{
	ASSERT(btmp != NULL);
	uint32_t byte_index = 0;
	uint8_t bits_index = 0;
	uint8_t byte_temp = 0;
	uint32_t unused_cnt = 0;
	while(byte_index < btmp->btmp_bytes_len)//逐字节判断过去
	{
		if(btmp->bits[byte_index] != 0xff)//该字节中有没被利用的位
		{
			byte_temp = btmp->bits[byte_index];//暂时保存该字节
			for(bits_index = 0; bits_index < 8; bits_index++)
			{
				if(byte_temp & 0x01)//当发现不可用位的时候
				{
					unused_cnt = 0;//需要重新开始计算
				}
			    else
				{
					unused_cnt++;
					if(unused_cnt == cnt)//满足要求
						return byte_index * 8 + bits_index - cnt + 1;
				}
				byte_temp = byte_temp >> 1;
			}
		}
		else//如果被利用了
		{
			unused_cnt = 0;
		}
		byte_index++;
	}
	return -1;
}

//将位图btmp的bit_idx位设置位val
void bitmap_set(struct bitmap* btmp, uint32_t bit_idx, int8_t val)
{
	ASSERT(val == 0 || val == 1);
	uint32_t byte_idx = bit_idx / 8;
	ASSERT(byte_idx < btmp->btmp_bytes_len);
	uint8_t bit_offset = bit_idx % 8;
	if(val)
		btmp->bits[byte_idx] |= (0x01 << bit_offset);
	else
		btmp->bits[byte_idx] &=~ (0x01 << bit_offset);
}


