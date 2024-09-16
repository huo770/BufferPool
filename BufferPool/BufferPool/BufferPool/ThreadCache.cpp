#define _CRT_SECURE_NO_WARNINGS

#include"ThreadCache.h"
#include"CentralCache.h"

//size��ʾ������ֽ���
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);          
	size_t alignSize = SizeClass::RoundUp(size);   //alignSzie ��ʾ�����ڴ�����ֽ���
	size_t index = SizeClass::Index(size);         //index��ʾ�ڴ���������е�����

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		//û���ڴ�飬��ȥcentrleCache��ȡ����
		return FetchFromCentraCache(index, alignSize);
	}
}

//ÿ���̶߳���һ��thread cache,��������������ֻ��һ��central cache�����Խ�central cache���óɵ���ģʽ������ģʽ������ģʽ
//����ʹ�õ����Ķ���
void* ThreadCache::FetchFromCentraCache(size_t index, size_t size)
{
	// ����ʼ���������㷨  -- С�������һ�㣬��������һ��
	//�ﵽ��Ŀ�ģ�
	//1.�ʼ����һ����central cacheһ������Ҫ̫�࣬��ΪҪ̫���ò���
	//2.����㲻Ҫ���size��С�ڴ�������ôbatchNum�ͻ᲻��������ֱ������
	//3.sizeԽ��һ����central cacheҪ��batchNum��ԽС
	//4.sizeԽС��һ����central cacheҪ��batchNum��Խ�����������ģ�
	size_t batchNum;
	if (_freeLists[index].MaxSize() < SizeClass::NumMoveSize(size))
		batchNum = _freeLists[index].MaxSize();
	else
		batchNum = SizeClass::NumMoveSize(size);
	//size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	//ͷ�ļ�windows.h����һ����Ҳ��min,min�Ǻ���ģ�壬����ֱ����windows.h�е��ˡ����Իᱨ��
	void* start = nullptr;
	void* end = nullptr;

	//������
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 1;
	}

	//actualNum��ʾʵ�ʸ���freelist�ĸ�����batchNum��ʾthread chacheҪ��freelist�ĸ���
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum > 1);

	//���ʵ��ֻ��ȡ��1��
	if (actualNum == 1)
	{
		assert(start == end);
		return start;
	}
	else
	{
		//�����ȡ���������Ӧ�ðѻ�ȡ�������������ͷ�ڵ㷵��ȥ
		_freeLists[index].PushRange(NextObj(start), end);
		return start;
	}
}
//�ͷ��ڴ�
void* ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	//������Ƕ��ٺ�Ͱ,�����ȥ
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	return nullptr;
}


//centralCache�ǹ���ģ�������Ҫ���������Ǽӵ���Ͱ����
//1���̷߳���1��Ͱ��2���̷߳���2��Ͱ���ǲ�Ҫ�����ģ���Ϊ�Ա˴˵�Ӱ�춼����