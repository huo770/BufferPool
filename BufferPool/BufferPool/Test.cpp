//#define _CRT_SECURE_NO_WARNINGS
#include"ObjectPool.h"

int main()
{
	Test1();
}
//#include "ConcurrentAlloc.h"
//
//void AllocTest1()
//{
//	//�õ��̲߳���
//	for (size_t i = 0; i < 5; ++i)
//	{
//		void* ptr = ConcurrentAlloc(6);
//	}
//}
//
//void AllocTest2()
//{
//	//�õ��̲߳���
//	for (size_t i = 0; i < 5; ++i)
//	{
//		void* ptr = ConcurrentAlloc(7);
//	}
//}
//void TLSTest()
//{
//	//����һ���߳�ȥִ��AllocAllocTest1����
//	std::thread t1(AllocTest1);
//	std::thread t2(AllocTest2);
//
//	t1.join();
//	t2.join();
//}
//
//void TestConcurrentAlloc()
//{
//	void* p1 = ConcurrentAlloc(6);
//	void* p2 = ConcurrentAlloc(7);
//	void* p3 = ConcurrentAlloc(8);
//	void* p4 = ConcurrentAlloc(1);
//}
//int main()
//{
//	//TestObjectPool();
//	//TLSTest();
//	TestConcurrentAlloc();
//	return 0;
//}