#pragma once
#include"Common.h"


template<class T>
class ObjectPool
{
public:
	//�����ڴ�
	T* New()
	{
		T* obj = nullptr;
		//1.���Ȱѻ��������ڴ���ٴ��ظ�����
		if (_freeList)  //�����л��������ڴ�����
		{
			//�ظ����ã����е������ͷɾ 
			//�����*(void**)_freeList��ʾȡfreeLiset��ǰָ���С���ֽ� -->Ҳ����ȡnext�ĵ�ַ
			void* next = *((void**)_freeList);     //?  ����Ϊʲôʹ��void**

			//obj��ȡ��������
			obj = (T*)_freeList;
			//�����ƶ�һλ
			_freeList = next;
		}
		else  //���û�л��������ڴ��
		{
			//�жϿ��ٵ�һ���ռ��Ƿ���������
			if (_remainBytes < sizeof(T))  
			{
				//�������
				_remainBytes = 1024 * 128;   //����128KB
				//����ΪʲôҪ��ôд?
				_memory = (char*)malloc(_remainBytes);
				//SystemAlloc��ϵͳ���ýӿ�
				//_memory = (char*)SystemAlloc(_remainBytes >> 13);
				
				//�쳣����
				if (_memory == nullptr)
				{
					throw std::bad_alloc();
				}
			}
			//����һ������ָ��memory,���objҲ�������������ڴ������ʵ��
			obj = (T*)_memory;

			//�����жϵ������ǣ������������Ŀռ����ָ��Ĵ�С���ͷ�����������Ŀռ䣬
			//���򷵻�ָ��Ĵ�С  --> ��������Ϊ���ڴ�������ܴ��µ�ַ
			size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);

			//����ڴ��ָ������ƶ�
			_memory += objSize;
			//���ÿռ����
			_remainBytes -= objSize;
		}

		new(obj) T;

		return obj;
	}

	//�����ͷŵĹ��̣����ͷŵ�С�ڴ��ҵ�_freeListǰ��
	void Delete(T* obj)
	{
		//��ʽ���ú����������
		obj->~T();

		//ͷ��
		//��obj�ڴ���ǰָ���С���ֽڴ���_freeList�ĵ�ַ
		*(void**)obj = _freeList;
		//��_freeList�ƶ�Ϊͷ���
		_freeList = obj;
	}
private:
	char* _memory = nullptr;         //ָ�����ڴ��ָ��
	size_t _remainBytes = 0;         //����ڴ��зֹ�����ʣ���ֽ���
	void* _freeList = nullptr;      //���������������ӵ����������ͷָ��
};


//����
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
	// �����ͷŵ��ִ�
	const size_t Rounds = 5;

	// ÿ�������ͷŶ��ٴ�
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

	cout << "ϵͳ�Դ���new cost time:" << end1 - begin1 << endl;
	cout << "�����ڴ�ص�object pool cost time:" << end2 - begin2 << endl;
}