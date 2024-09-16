#pragma once
#include "Common.h"

class CentralCache
{
public:
	//��ȡʵ������
	static CentralCache* GetInstance()
	{
		return &_sInst;
	}

	//��ȡһ���ǿյ�span
	Span* GetOneSpan(SpanList& list, size_t size);

	//�����Ļ��ӻ�ȡһ�������Ķ����thread cache
	//ȡn�� byte_size��С��������ʼΪstart,��ֹΪend
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t size);
private:
	SpanList _spanlists[NFREELIST];    //��thread cache���Ǽ���Ͱ����central cache��Ҳ�Ǽ���Ͱ
private:
	//����ģʽ����Ҫ����Ҳʹ������������Խ����캯��˽��
	CentralCache()
	{}

	//��������Ҳ˽��
	CentralCache(const CentralCache&) = delete;
	static CentralCache _sInst;
};


//������ȣ�thread cacheû���ڴ��ˣ�Ҫ��CentralCacheҪ��
//thread cache���ڴ�̫���ˣ�Ҫ�Ż�CentralCache��ͬʱCentralCache���Ը�������thread cache��.