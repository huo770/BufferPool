#pragma once
#include<iostream>
#include<vector>
#include<time.h>
#include<assert.h>
#include<thread>
#include<mutex>
#include <Windows.h>

using std::cout;
using std::endl;

//为什么使用static?是为了不改变 -- 尽量不使用宏
static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208;   //总共的哈希桶的数量
static const size_t NPAGES = 128;
static const size_t PAGE_SHIFT = 13;   //为什么是13，因为一页是8k， 2的13次方

#ifdef _WIN32
	typedef size_t PAGE_ID;
#elif _WIN64
	typedef unsigned long long PAGE_ID;
#endif


	//直接去堆上按页申请空间
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
		//linux下用mmp或者brk
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

//封装一个函数，取前4/8字节的大小
static void*& NextObj(void* obj)
{
	return *(void**)obj;
}
//管理切分好的小对象的自由链表
class FreeList
{
public:
	//插入一个对象
	void Push(void* obj)
	{
		//头插
		NextObj(obj) = _freeList;  //等价于obj->next = _freeList
		_freeList = obj;
	}
	
	//批量插入freelist,头插
	void PushRange(void* start, void* end)
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}
	//从自由链表中删除内存
	void* Pop()
	{
		assert(_freeList);
		//头删
		void* obj = _freeList;
		_freeList = NextObj(obj);   //等价于_freeList = obj->next 

		//头删，指向下一个，所以返回obj
		return obj;
	}
	
	//判断是否为空
	bool Empty()
	{
		return _freeList == nullptr;
	}

	size_t& MaxSize()
	{
		return _maxSize;
	}
private:
	void* _freeList = nullptr;
	size_t _maxSize = 1;
};


//计算对象大小的对齐映射规则
class SizeClass
{
public:
	//子函数
	//size_t _RoundUp(size_t size, size_t AlignNum)
	//{
	//	size_t alignSize;
	//	if (size % AlignNum != 0)
	//	{
	//		alignSize = (size / AlignNum + 1) * AlignNum;
	//	}
	//	else
	//	{
	//		alignSize = size;
	//	}

	//	return alignSize;
	//}

	//计算给定字节数，根据内存对齐规则，最终应该返回的字节数
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + (alignNum)-1) & ~(alignNum - 1));
	}

	//对齐规则：
	static size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);   //八字节对齐
		}
		else if (size <= 1024)
		{
			return _RoundUp(size, 16);
		}
		else if (size <= 8 * 1024)
		{
			return _RoundUp(size, 128);
		}
		else if (size <= 64 * 1024)
		{
			return _RoundUp(size, 1024);
		}
		else if (size <= 256 * 1024)
		{
			return _RoundUp(size, 8 * 1024);
		}
		else
		{
			assert(size);
		}

	}

	//static inline size_t _Index(size_t bytes, size_t alignNum)
	//{
	//	if (bytes % alignNum == 0)
	//	{
	//		return bytes / alignNum - 1;
	//	}
	//	else
	//	{
	//		return bytes / alignNum;
	//	}
	//}

	static inline size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + (1 << align_shift) - 1) >> align_shift) + 1;
	}

	//计算映射的是哪一个自由链表桶
	static inline size_t Index(size_t Bytes)
	{
		assert(Bytes <= MAX_BYTES);

		//对应的每一个分区有多少个桶
		static int group_array[4] = { 16, 56, 56, 56 };
		if (Bytes <= 128)
		{
			return _Index(Bytes, 3);
		}
		else if (Bytes <= 1024)
		{
			//根据不同的映射规则
			//Bytes - 128表示在第二个区间有多少个字节
			//加group_array[0]表示相对第一个区间的偏移量，第一个区间有16个桶，所以group_array[0] = 16
			return _Index(Bytes - 128, 4) + group_array[0];
		}
		else if (Bytes <= 8 * 1024)
		{
			return _Index(Bytes - 8 * 1024, 7) + group_array[1] + group_array[0];
		}
		else if (Bytes <= 64 * 1024)
		{
			return _Index(Bytes - 64 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (Bytes <= 256 * 1024)
		{
			return _Index(Bytes - 256 * 1024, 13) + group_array[2] + group_array[2] +
				group_array[1] + group_array[0];
		}
	}

	static size_t NumMoveSize(size_t size)   //thread cache要的内存大小
	{
		assert(size > 0);
		//[2, 512] 一次批量移动多少个对象的（慢启动)上限值
		//小对象一次批量上限高
		//大对象一次批量上限低

		int num = MAX_BYTES / size;
		if (num < 2)
		{
			num = 2;
		}
		
		if (num > 512)
		{
			num = 512;
		}

		return num;
	}

	//计算一次向系统获取几个页
	//单个对象 8byte
	//....
	//单个对象256byte
	static size_t NumMovepage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;   //npage表示算出来的字节数

		npage >>= PAGE_SHIFT;        //这里npage表示算出来的页数
		if (npage == 0)
			npage = 1;
		
		return npage;
	}
};


struct Span
{
	size_t _pageid = 0;      //大块内存起始页的也好
	size_t _n = 0;            //页的数量

	Span* _next = nullptr;         //双向链表的结构
	Span* _prev = nullptr;

	size_t _useCount = 0;    //切好小块内存，被分配给thread cache的计数
	void* _freeList = nullptr;     //切好的小块内存的自由链表
};

//带头双向循环链表
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
	}

	bool Empty()
	{
		return _head->_next == _head;
	}
	void PushFront(Span* span)
	{
		Insert(Begin(), span);
	}

	Span* PopFront()
	{
		Span* front = _head->_next;
		Erase(front);
		return front;
	}
	//插入
	void Insert(Span* pos, Span* newSpan)
	{
		assert(pos);
		assert(newSpan);

		Span* prev = pos->_prev;

		prev->_next = newSpan;
		newSpan->_prev = prev;
		newSpan->_next = pos;
		pos->_prev = newSpan;
	}
	//删除
	void Erase(Span* pos)
	{
		assert(pos);
		assert(pos != _head);
		Span* prev = pos->_prev;
		Span* next = pos->_next;

		prev->_next = next;
		next->_prev = prev;
	}
private:
	Span* _head;
public:
	std::mutex _mtx;      //桶锁，当两个线程访问同一个桶时，有竞争
};