#pragma once

#include"Common.h"


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

	//��ȡһ��kҳ��span
	Span* NewSpan(size_t k);

	std::mutex _pageMtx;
private:
	//page cache����ҳ������ӳ��
	SpanList _spanLists[NPAGES];
	
	PageCache()
	{}

	PageCache(const PageCache&) = delete;

	static PageCache _sInst;
};