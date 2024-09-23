#pragma once

#include"Common.h"
#include "ObjectPool.h"
#include "PageMap.h"

//最开始page cache没有任何span,在central cache向page cache要的时候
//page cache向系统申请一个128page span
//page cache将2page span给central cache,自己还剩126page span,就挂载到126page下


//当central cache中的usecount为0时，就代表thread cache将所有的内存都还回来了
//central cache就将span都还给page cache

//page cache通过页号开始往前和往后遍历，把空闲的page合并，解决内存碎片问题
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	// 获取从对象到span的映射
	Span* MapObjectToSpan(void* obj);

	// 释放空闲span回到Pagecache，并合并相邻的span
	void ReleaseSpanToPageCache(Span* span);
	//获取一个k页的span
	Span* NewSpan(size_t k);

	std::mutex _pageMtx;
private:
	//page cache按照页数进行映射
	SpanList _spanLists[NPAGES];
	ObjectPool<Span> _spanPool;
	
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//std::map<PAGE_ID, Span*> _idSpanMap;
	TCMalloc_PageMap1<32 - PAGE_SHIFT> _idSpanMap;
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};