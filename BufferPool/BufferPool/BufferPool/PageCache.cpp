#define _CRT_SECURE_NO_WARNINGS

#include"PageCache.h"

PageCache PageCache::_sInst;
//��ȡһ��kҳ��span
Span* PageCache::NewSpan(size_t k)
{
	assert(k > 0 && k <= NPAGES);

	if (!_spanLists[k].Empty())
	{
		return _spanLists->PopFront();
	}

	//˵����k��Ͱ�ǿյģ���ȥ�������Ͱ������û��span
	//����У����԰Ѵ��span�����з�
	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		if (!_spanLists[i].Empty())
		{
			Span* nSpan = _spanLists[i].PopFront();

			Span* kSpan = new Span;

			//��nSpan��ͷ����һ��kҳ����
			//kҳ��span����
			//nspan�ٹҵ���Ӧӳ���λ��
			kSpan->_pageid = nSpan->_pageid;
			kSpan->_n = k;

			nSpan->_pageid += k;
			nSpan->_next -= k;

			_spanLists[nSpan->_n].PushFront(nSpan);
			return kSpan;
		}
		
		//�ߵ����˵������û�д�ҳ��span��
		//��ʱ��ȥ�Ҷ�Ҫһ��128ҳ��span
		Span* bigSpan = new Span;
		void* ptr = SystemAlloc(NPAGES - 1);
		//ͨ��ָ����μ���ҳ��
		bigSpan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
		//ҳ���Ƕ���
		bigSpan->_n = NPAGES - 1;


		//��bigSpan���뵽��Ӧ��Ͱ�
		_spanLists[bigSpan->_n].PushFront(bigSpan);
		//�ݹ�����Լ� --> Ϊ�˲��ظ�д����Ĵ���
		return NewSpan(k);
	}
}