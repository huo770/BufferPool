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

//Ϊʲôʹ��static?��Ϊ�˲��ı� -- ������ʹ�ú�
static const size_t MAX_BYTES = 256 * 1024;
static const size_t NFREELIST = 208;   //�ܹ��Ĺ�ϣͰ������
static const size_t NPAGES = 128;
static const size_t PAGE_SHIFT = 13;   //Ϊʲô��13����Ϊһҳ��8k�� 2��13�η�

#ifdef _WIN32
	typedef size_t PAGE_ID;
#elif _WIN64
	typedef unsigned long long PAGE_ID;
#endif


	//ֱ��ȥ���ϰ�ҳ����ռ�
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, kpage << 13, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
		//linux����mmp����brk
#endif
	if (ptr == nullptr)
		throw std::bad_alloc();

	return ptr;
}

//��װһ��������ȡǰ4/8�ֽڵĴ�С
static void*& NextObj(void* obj)
{
	return *(void**)obj;
}
//�����зֺõ�С�������������
class FreeList
{
public:
	//����һ������
	void Push(void* obj)
	{
		//ͷ��
		NextObj(obj) = _freeList;  //�ȼ���obj->next = _freeList
		_freeList = obj;
	}
	
	//��������freelist,ͷ��
	void PushRange(void* start, void* end)
	{
		NextObj(end) = _freeList;
		_freeList = start;
	}
	//������������ɾ���ڴ�
	void* Pop()
	{
		assert(_freeList);
		//ͷɾ
		void* obj = _freeList;
		_freeList = NextObj(obj);   //�ȼ���_freeList = obj->next 

		//ͷɾ��ָ����һ�������Է���obj
		return obj;
	}
	
	//�ж��Ƿ�Ϊ��
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


//��������С�Ķ���ӳ�����
class SizeClass
{
public:
	//�Ӻ���
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

	//��������ֽ����������ڴ�����������Ӧ�÷��ص��ֽ���
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + (alignNum)-1) & ~(alignNum - 1));
	}

	//�������
	static size_t RoundUp(size_t size)
	{
		if (size <= 128)
		{
			return _RoundUp(size, 8);   //���ֽڶ���
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

	//����ӳ�������һ����������Ͱ
	static inline size_t Index(size_t Bytes)
	{
		assert(Bytes <= MAX_BYTES);

		//��Ӧ��ÿһ�������ж��ٸ�Ͱ
		static int group_array[4] = { 16, 56, 56, 56 };
		if (Bytes <= 128)
		{
			return _Index(Bytes, 3);
		}
		else if (Bytes <= 1024)
		{
			//���ݲ�ͬ��ӳ�����
			//Bytes - 128��ʾ�ڵڶ��������ж��ٸ��ֽ�
			//��group_array[0]��ʾ��Ե�һ�������ƫ��������һ��������16��Ͱ������group_array[0] = 16
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

	static size_t NumMoveSize(size_t size)   //thread cacheҪ���ڴ��С
	{
		assert(size > 0);
		//[2, 512] һ�������ƶ����ٸ�����ģ�������)����ֵ
		//С����һ���������޸�
		//�����һ���������޵�

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

	//����һ����ϵͳ��ȡ����ҳ
	//�������� 8byte
	//....
	//��������256byte
	static size_t NumMovepage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t npage = num * size;   //npage��ʾ��������ֽ���

		npage >>= PAGE_SHIFT;        //����npage��ʾ�������ҳ��
		if (npage == 0)
			npage = 1;
		
		return npage;
	}
};


struct Span
{
	size_t _pageid = 0;      //����ڴ���ʼҳ��Ҳ��
	size_t _n = 0;            //ҳ������

	Span* _next = nullptr;         //˫������Ľṹ
	Span* _prev = nullptr;

	size_t _useCount = 0;    //�к�С���ڴ棬�������thread cache�ļ���
	void* _freeList = nullptr;     //�кõ�С���ڴ����������
};

//��ͷ˫��ѭ������
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
	//����
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
	//ɾ��
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
	std::mutex _mtx;      //Ͱ�����������̷߳���ͬһ��Ͱʱ���о���
};