#define _CRT_SECURE_NO_WARNINGS

#include"PageCache.h"

PageCache PageCache::_sInst;
//获取一个k页的span
Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k <= NPAGES);

	if (!_spanLists[k].Empty())
	{
		return _spanLists->PopFront();
	}

	//说明第k个桶是空的，再去检查后面的桶里面有没有span
	//如果有，可以把大的span进行切分
	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();

			Span* kSpan = new Span;

			//在nSpan的头部切一个k页下来
			//k页的span返回
			//nspan再挂到对应映射的位置
			kSpan->_pageid = nSpan->_pageid;
			kSpan->_n = k;

			nSpan->_pageid += k;
			nSpan->_next -= k;

			_spanLists[nSpan->_n].PushFront(nSpan);
			return kSpan;
		}
		
		//走到这里，说明后面没有大页的span了
		//这时就去找堆要一个128页的span
		Span* bigSpan = new Span;
		void* ptr = SystemAlloc(NPAGES - 1);
		//通过指针如何计算页号
		bigSpan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
		//页数是多少
		bigSpan->_n = NPAGES - 1;


		//将bigSpan插入到对应的桶里，
		_spanLists[bigSpan->_n].PushFront(bigSpan);
		//递归调用自己 --> 为了不重复写上面的代码
		return NewSpan(k);
	}
}