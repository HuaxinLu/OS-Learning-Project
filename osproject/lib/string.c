#include "string.h"
#include "global.h"
#include "debug.h"

void memset(void* dst_, uint8_t val, uint32_t size)
{
	ASSERT(dst_ != NULL);
	uint8_t* dst = (uint8_t*)dst_;
	while(size-- > 0)
		*dst++ = val;
}

void memcpy(void* dst_, const void* src_, uint32_t size)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	uint8_t* dst = (uint8_t*)dst_;
	const uint8_t* src = (uint8_t*)src_;
	while(size-- > 0)
		*dst++ = *src++;
}

//连续比较a和b开头的size个字节，如果某一个字节中，a大于b则返回1,都相等返回0，小于返回-1
int memcmp(const void* a_, const void* b_, uint32_t size)
{
	ASSERT(a_ != NULL && b_ != NULL);
	const char* a = (const char*)a_;
	const char* b = (const char*)b_;
	while(size-- > 0)
	{
		if(*a > *b) return 1;
		if(*a < *b) return -1;
		a++;
		b++;
	}
	return 0;
}

char* strcpy(char* dst_, const char* src_)
{
	ASSERT(dst_ != NULL && src_ != NULL);
	char* dst = dst_;
	while(*src_ != '\0')
		*dst++ = *src_++;
	return dst_;
}

uint32_t strlen(const char* str)
{
	ASSERT(str != NULL);
	const char* p = str;
	while(*p++);
	return p - str - 1;
}

int8_t strcmp(const char* a, const char* b)
{
	ASSERT(a != NULL && b != NULL);
	while(*a != '\0' && *a == *b)
	{
		a++;
		b++;
	}
	return *a < *b ? -1 : *a > *b;
}

//首次出现ch的地址
char* strchr(const char* str, char ch)
{
	ASSERT(str != NULL);
	while(*str != '\0')
	{
		if(*str == ch)
			return (char*)str;
		str++;
	}
	return NULL;
}

//从后往前查找字符串中首次出现ch的地址
char* strrchr(const char* str, char ch)
{
	ASSERT(str != NULL);
	const char* last_char = NULL;
	while(str != '\0')
	{
		if(*str == ch)
			last_char = str;
		str++;
	}
	return (char*)last_char;
}

//字符串拼接
char* strcat(char* dst_, const char* src)
{
	ASSERT(dst_ != NULL && src != NULL);
	char* dst = dst_;
	while(*dst++ != '\0');
	dst--;
	while((*dst++ = *src++) != '\0');
	return dst_;
}

//查找ch出现的次数
uint32_t strchrs(const char* str, char ch)
{
	ASSERT(str != NULL);
	uint32_t cnt = 0;
	while(*str != '\0')
	{
		if(*str == ch)
			cnt++;
		str++;
	}
	return cnt;
}




