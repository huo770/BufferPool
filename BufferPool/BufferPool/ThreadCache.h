#pragma once
#include"Common.h"

class ThreadCache
{
public:
	//申请内存
	void* Allocate(size_t size);
	//释放内存
	void Deallocate(void* ptr, size_t size);
	//从中心缓存获取内存
	void* FetchFromCentralCache(size_t index, size_t size);
	// 释放对象时，链表过长时，回收内存回到中心缓存
	void ListTooLong(FreeList& list, size_t size);
private:
	//需要一个_freeList的哈希桶
	FreeList _freeLists[NFREELIST];
};

//每个线程都有自己的ThreadCache  -- 好处是不用加锁了
//每个线程都有pTLSpTLSThreadCache指针
//TLS是一种变量的存储方式，这个变量在它所在的线程中是全局可访问的，但是不能被其他线程访问，保持了数据的线程独立性。
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;