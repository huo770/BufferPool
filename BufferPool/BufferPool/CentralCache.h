#pragma once
#include "Common.h"

class CentralCache
{
public:
	//获取实例对象
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	//获取一个非空的span
	Span* GetOneSpan(SpanList& list, size_t size);

	//从中心换从获取一定数量的对象给thread cache
	//取n个 byte_size大小的链表，起始为start,终止为end
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t size);
private:
	SpanList _spanlists[NFREELIST];    //在thread cache中是几号桶，在central cache中也是几号桶
private:
	//单例模式不想要别人也使用这个对象，所以将构造函数私有
	CentralCache()
	{}

	//拷贝构造也私有
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
};


//均衡调度：thread cache没有内存了，要找CentralCache要。
//thread cache得内存太多了，要放回CentralCache。同时CentralCache可以给其他的thread cache用.