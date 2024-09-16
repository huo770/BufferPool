#define _CRT_SECURE_NO_WARNINGS
#include "CentralCache.h"
#include "PageCache.h"

//要在.cpp中定义，.h中是声明
CentralCache CentralCache::_sInst;

//获取一个非空的span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//查看当前的spanlist中是否还有未分配对象的span
	Span* it = list.Begin();
	while (it != list.End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}
	//先把central cache的桶锁解掉，这样如果其他线程释放内存对象回来，不会阻塞
	list._mtx.unlock();

	PageCache::GetInstance()->_pageMtx.lock();
	//走到这里，说明没有空闲span,只能找page cache要
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovepage(size));  //size越大，分配的页越大；size越小，分配的页越小
	PageCache::GetInstance()->_pageMtx.unlock();

	//对获取span进行切分，不需要加锁，因为这会儿其他线程访问不到这个span
	//计算span的大块内存的起始地址和大块内存的大小（字节数)
	char* start = (char*)(span->_pageid << PAGE_SHIFT);   //start表示页的起始地址
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//把大块内存切成自由链表链接起来
	//1.先切一块下来去做头，方便尾插
	span->_freeList = start;
	start += size;
	//2.
	void* tail = span->_freeList;

	while (start < end)
	{
		NextObj(tail) = start;
		tail = start;
		start += size;
	}

	list.PushFront(span);
	return span;
}
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchNum, size_t size)
{
	size_t index = SizeClass::Index(size);
	//有可能多个线程去竞争同一个桶，所以要加锁
	_spanlists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanlists[index], size);
	assert(span);
	assert(span->_freeList);

	//从span中获取batchNum个对象，如果不够batchNum，有多少拿多少
	start = span->_freeList;
	end = start;
	size_t i = 0;
	size_t actualNum = 1;
	while ( i < batchNum - 1 && NextObj(end) != nullptr)//防止要的batchNum 大于已有的_freelist个数的情况（发生越界）
	{
		end = NextObj(end);
		++i;
		++actualNum;
	}
	span->_freeList = NextObj(end);
	NextObj(end) = nullptr;

	_spanlists[index]._mtx.unlock();

	return actualNum;
}