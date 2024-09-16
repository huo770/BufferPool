#pragma once
#include"Common.h"


template<class T>
class ObjectPool
{
public:
	//申请内存
	T* New()
	{
		T* obj = nullptr;
		//1.优先把换回来的内存块再次重复利用
		if (_freeList)  //代表有还回来的内存块对象
		{
			//重复利用，进行单链表的头删 
			//这里的*(void**)_freeList表示取freeLiset的前指针大小个字节 -->也就是取next的地址
			void* next = *((void**)_freeList);     //?  这里为什么使用void**

			//obj是取整个对象
			obj = (T*)_freeList;
			//往后移动一位
			_freeList = next;
		}
		else  //如果没有还回来的内存块
		{
			//判断开辟的一大块空间是否够这次申请的
			if (_remainBytes < sizeof(T))  
			{
				//如果不够
				_remainBytes = 1024 * 128;   //申请128KB
				//这里为什么要这么写?
				_memory = (char*)malloc(_remainBytes);
				//SystemAlloc是系统调用接口
				//_memory = (char*)SystemAlloc(_remainBytes >> 13);
				
				//异常处理
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			//创建一个对象指向memory,这个obj也就是我们申请内存给到的实体
			obj = (T*)_memory;

			//这里判断的意义是：如果我们申请的空间大于指针的大小，就返回我们申请的空间，
			//否则返回指针的大小  --> 这样做是为了内存块至少能存下地址
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);

			//大块内存的指针向后移动
			_memory += objSize;
			//可用空间减少
			_remainBytes -= objSize;
		}

		new(obj) T;

		return obj;
	}

	//这是释放的过程，将释放的小内存块挂到_freeList前面
	void Delete(T* obj)
	{
		//显式调用函数清理对象
		obj->~T();

		//头插
		//将obj内存块的前指针大小的字节存入_freeList的地址
		*(void**)obj = _freeList;
		//将_freeList移动为头结点
		_freeList = obj;
	}
private:
	char* _memory = nullptr;         //指向大块内存的指针
	size_t _remainBytes = 0;         //大块内存切分过程中剩余字节数
	void* _freeList = nullptr;      //还回来过程中链接的滋有链表的头指针
};


//测试
struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;

	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void TestObjectPool()
{
	// 申请释放的轮次
	const size_t Rounds = 5;

	// 每轮申请释放多少次
	const size_t N = 100000;

	std::vector<TreeNode*> v1;
	v1.reserve(N);

	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}

	size_t end1 = clock();

	std::vector<TreeNode*> v2;
	v2.reserve(N);

	ObjectPool<TreeNode> TNPool;
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}
	size_t end2 = clock();

	cout << "系统自带的new cost time:" << end1 - begin1 << endl;
	cout << "定长内存池的object pool cost time:" << end2 - begin2 << endl;
}