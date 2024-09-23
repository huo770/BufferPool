#pragma once

#include"Common.h"
#include "ObjectPool.h"
#include "PageMap.h"

//�ʼpage cacheû���κ�span,��central cache��page cacheҪ��ʱ��
//page cache��ϵͳ����һ��128page span
//page cache��2page span��central cache,�Լ���ʣ126page span,�͹��ص�126page��


//��central cache�е�usecountΪ0ʱ���ʹ���thread cache�����е��ڴ涼��������
//central cache�ͽ�span������page cache

//page cacheͨ��ҳ�ſ�ʼ��ǰ������������ѿ��е�page�ϲ�������ڴ���Ƭ����
class PageCache
{
public:
	static PageCache* GetInstance()
	{
		return &_sInst;
	}

	// ��ȡ�Ӷ���span��ӳ��
	Span* MapObjectToSpan(void* obj);

	// �ͷſ���span�ص�Pagecache�����ϲ����ڵ�span
	void ReleaseSpanToPageCache(Span* span);
	//��ȡһ��kҳ��span
	Span* NewSpan(size_t k);

	std::mutex _pageMtx;
private:
	//page cache����ҳ������ӳ��
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