#pragma once
#include "Common.h"
#include "ThreadCache.h"

//多个线程都来调用，那么每个线程如何获取到自己的ThreadCache呢？
void* ConcurrentAlloc(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		pTLSThreadCache = new ThreadCache;
	}

	return pTLSThreadCache->Allocate(size);
}

static void ConcurrentFree(void* ptr, size_t size)
{
	assert(pTLSThreadCache);

	pTLSThreadCache->Deallocate(ptr, size);
}