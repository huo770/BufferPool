#pragma once
#include"Common.h"

class ThreadCache
{
public:
	//�����ڴ�
	void* Allocate(size_t size);
	//�ͷ��ڴ�
	void Deallocate(void* ptr, size_t size);
	//�����Ļ����ȡ�ڴ�
	void* FetchFromCentralCache(size_t index, size_t size);
	// �ͷŶ���ʱ���������ʱ�������ڴ�ص����Ļ���
	void ListTooLong(FreeList& list, size_t size);
private:
	//��Ҫһ��_freeList�Ĺ�ϣͰ
	FreeList _freeLists[NFREELIST];
};

//ÿ���̶߳����Լ���ThreadCache  -- �ô��ǲ��ü�����
//ÿ���̶߳���pTLSpTLSThreadCacheָ��
//TLS��һ�ֱ����Ĵ洢��ʽ����������������ڵ��߳�����ȫ�ֿɷ��ʵģ����ǲ��ܱ������̷߳��ʣ����������ݵ��̶߳����ԡ�
static _declspec(thread) ThreadCache* pTLSThreadCache = nullptr;