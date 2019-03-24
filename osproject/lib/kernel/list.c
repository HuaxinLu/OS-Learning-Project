#include "list.h"
#include "interrupt.h"
#include "debug.h"

//初始化链表
void list_init(struct list* plist)
{
	plist->head.prev = NULL;
	plist->head.next = &plist->tail;
	plist->tail.prev = &plist->head;
	plist->tail.next = NULL;
}
//把元素elem插在before之前
void list_insert_before(struct list_elem* before, struct list_elem* elem)
{
	ASSERT(before != NULL && before->prev != NULL  && elem != NULL);
	enum intr_status old_status = intr_disable();
	before->prev->next = elem;
	elem->prev = before->prev;
	elem->next = before;
	before->prev = elem;
	intr_set_status(old_status);
}
//添加元素到列表队首，类似push操作
void list_push(struct list* plist, struct list_elem* elem)
{
	list_insert_before(plist->head.next, elem);	
}
//添加元素队列队尾
void list_append(struct list* plist, struct list_elem* elem)
{
	list_insert_before(&plist->tail, elem);
}
//删除节点
void list_remove(struct list_elem* elem)
{
	ASSERT(elem != NULL);
	enum intr_status old_status = intr_disable();
	if(elem->prev != NULL)
		elem->prev->next = elem->next;
	if(elem->next != NULL)
		elem->next->prev = elem->prev;
	intr_set_status(old_status);	
}
//链表的第一个元素弹出并返回
struct list_elem* list_pop(struct list* plist)
{
	ASSERT(plist != NULL);
	struct list_elem* temp = plist->head.next;
	list_remove(temp);
	return temp;
}
//查找元素
bool elem_find(struct list* plist, struct list_elem* elem)
{
	ASSERT(plist != NULL && elem != NULL);
	struct list_elem* temp = plist->head.next;
	while(temp != &plist->tail)
	{
		if(temp == elem)
			return 1;
		temp = temp->next;
	}
	return 0;
}
//遍历
struct list_elem* list_traversal(struct list* plist, function func, int arg)
{
	ASSERT(plist != NULL);
	if(list_empty(plist))
		return NULL;
	struct list_elem* temp = plist->head.next;
	while(temp != &plist->tail)
	{
		if(func(temp, arg) == 1)
			return temp;
		temp = temp->next;
	}
	return NULL;
}
//返回链表长度
uint32_t list_len(struct list* plist)
{
	ASSERT(plist != NULL);
	uint32_t len = 0;
	struct list_elem* temp = plist->head.next;
	while(temp != &plist->tail)
	{
		len++;
		temp = temp->next;
	}
	return len;
}
//判断链表是否空
bool list_empty(struct list* plist)
{
	ASSERT(plist != NULL);
	return plist->head.next == &plist->tail ? true : false;
}













