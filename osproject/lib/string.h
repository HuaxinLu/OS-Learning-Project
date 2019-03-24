#ifndef __LIB_STRING_H
#define __LIB_STRING_H

#include "stdint.h"

void memset(void* dst_, uint8_t val, uint32_t size);
void memcpy(void* dst_, const void* src_, uint32_t size);
int memcmp(const void* a_, const void* b_, uint32_t size);
uint32_t strlen(const char* str);
int8_t strcmp(const char* a, const char* b);
char* strcpy(char* dst_, const char* src_);
char* strchr(const char* str, char ch);
char* strrchr(const char* str, char ch);
char* strcat(char* dst_, const char* src);
uint32_t strchrs(const char* str, char ch);



#endif
