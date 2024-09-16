#define _CRT_SECURE_NO_WARNINGS

#include"ThreadCache.h"
#include"CentralCache.h"

//size表示申请的字节数
void* ThreadCache::Allocate(size_t size)
{
	assert(size <= MAX_BYTES);          
	size_t alignSize = SizeClass::RoundUp(size);   //alignSzie 表示最终内存给的字节数
	size_t index = SizeClass::Index(size);         //index表示内存块在数组中的索引

	if (!_freeLists[index].Empty())
	{
		return _freeLists[index].Pop();
	}
	else
	{
		//没有内存块，就去centrleCache获取对象
		return FetchFromCentraCache(index, alignSize);
	}
}

//每个线程都有一个thread cache,但是整个进程中只有一个central cache，所以将central cache设置成单例模式：饿汉模式，懒汉模式
//这里使用单例的饿汉
void* ThreadCache::FetchFromCentraCache(size_t index, size_t size)
{
	// 满开始反馈调节算法  -- 小对象给多一点，大对象给少一点
	//达到的目的：
	//1.最开始不会一次向central cache一次批量要太多，因为要太多用不完
	//2.如果你不要这个size大小内存需求，那么batchNum就会不断增长，直到上限
	//3.size越大，一次向central cache要的batchNum就越小
	//4.size越小，一次向central cache要的batchNum就越大（慢慢增长的）
	size_t batchNum;
	if (_freeLists[index].MaxSize() < SizeClass::NumMoveSize(size))
		batchNum = _freeLists[index].MaxSize();
	else
		batchNum = SizeClass::NumMoveSize(size);
	//size_t batchNum = std::min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));
	//头文件windows.h中有一个宏也叫min,min是函数模板，所以直接用windows.h中的了。所以会报错
	void* start = nullptr;
	void* end = nullptr;

	//慢增长
	if (batchNum == _freeLists[index].MaxSize())
	{
		_freeLists[index].MaxSize() += 1;
	}

	//actualNum表示实际给的freelist的个数，batchNum表示thread chache要的freelist的个数
	size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchNum, size);
	assert(actualNum > 1);

	//如果实际只获取到1个
	if (actualNum == 1)
	{
		assert(start == end);
		return start;
	}
	else
	{
		//如果获取到多个，就应该把获取到的自由链表的头节点返回去
		_freeLists[index].PushRange(NextObj(start), end);
		return start;
	}
}
//释放内存
void* ThreadCache::Deallocate(void* ptr, size_t size)
{
	assert(ptr);
	assert(size <= MAX_BYTES);

	//算出来是多少号桶,插入进去
	size_t index = SizeClass::Index(size);
	_freeLists[index].Push(ptr);

	return nullptr;
}


//centralCache是共享的，所以需要加锁，但是加的是桶锁。
//1号线程访问1号桶，2号线程访问2号桶，是不要加锁的，因为对彼此的影响都不大