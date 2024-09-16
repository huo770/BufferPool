#define _CRT_SECURE_NO_WARNINGS
#include "CentralCache.h"
#include "PageCache.h"

//Ҫ��.cpp�ж��壬.h��������
CentralCache CentralCache::_sInst;

//��ȡһ���ǿյ�span
Span* CentralCache::GetOneSpan(SpanList& list, size_t size)
{
	//�鿴��ǰ��spanlist���Ƿ���δ��������span
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
	//�Ȱ�central cache��Ͱ�������������������߳��ͷ��ڴ�����������������
	list._mtx.unlock();

	PageCache::GetInstance()->_pageMtx.lock();
	//�ߵ����˵��û�п���span,ֻ����page cacheҪ
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovepage(size));  //sizeԽ�󣬷����ҳԽ��sizeԽС�������ҳԽС
	PageCache::GetInstance()->_pageMtx.unlock();

	//�Ի�ȡspan�����з֣�����Ҫ��������Ϊ���������̷߳��ʲ������span
	//����span�Ĵ���ڴ����ʼ��ַ�ʹ���ڴ�Ĵ�С���ֽ���)
	char* start = (char*)(span->_pageid << PAGE_SHIFT);   //start��ʾҳ����ʼ��ַ
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//�Ѵ���ڴ��г�����������������
	//1.����һ������ȥ��ͷ������β��
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
	//�п��ܶ���߳�ȥ����ͬһ��Ͱ������Ҫ����
	_spanlists[index]._mtx.lock();

	Span* span = GetOneSpan(_spanlists[index], size);
	assert(span);
	assert(span->_freeList);

	//��span�л�ȡbatchNum�������������batchNum���ж����ö���
	start = span->_freeList;
	end = start;
	size_t i = 0;
	size_t actualNum = 1;
	while ( i < batchNum - 1 && NextObj(end) != nullptr)//��ֹҪ��batchNum �������е�_freelist���������������Խ�磩
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