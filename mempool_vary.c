#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <math.h> */
#include <errno.h>

#include "pdebug.h"
#include "mempool.h"
#include "mempool_vary.h"

struct vari_mempool {
	size_t max_objsize;
	int nr_pools;
	memory_pool_t* pools[0];
};

#define MIN_DATA_SIZE		sizeof(memory_pool_data_t)
#define MIN_PAGE_SIZE		1024
#define MIN_PAGE_N_OBJ		4
#define OBJSIZE_OF_POOL_N(n)	(MIN_DATA_SIZE << (n))

/* map different sized objects into different pool */
static int pool_index(size_t size) {
	int r = (size - 1) / MIN_DATA_SIZE + 1; /* r * MIN_DATA_SIZE >= size */
	int i, len;
	for (i = 0, len = 1; len > 0; i++, len <<= 1) {
		if (len >= r) break;
	}
	return i;		/* (1 << i) >= r  */
}

/* calculate data size in the pools */
static size_t pooled_datasize(size_t size)
{
	int n = pool_index(size);
	return OBJSIZE_OF_POOL_N(n);
}

static memory_pool_t* vari_mempool_get_pool(vari_mempool_t* pool, size_t objsize)
{
	if (objsize > pool->max_objsize) { /* not in pools */
		return NULL;
	}
	int idx = pool_index(objsize);
	return pool->pools[idx];
}

static memory_pool_t* alloc_new_memory_pool(int idx)
{
	size_t objsize = OBJSIZE_OF_POOL_N(idx);
	int page_n_obj = (MIN_PAGE_SIZE - MIN_DATA_SIZE) / objsize;
	if (page_n_obj < MIN_PAGE_N_OBJ) page_n_obj = MIN_PAGE_N_OBJ;
	return memory_pool_new(objsize, page_n_obj, 1); /* always lock */
}

vari_mempool_t* vari_mempool_new(struct vari_mempool_cfg* cfg)
{
	int nr_pools = pool_index(cfg->max_objsize_alloc) + 1;
	vari_mempool_t* pool = malloc(sizeof(vari_mempool_t) + nr_pools * sizeof(memory_pool_t*));
	if (!pool) goto end;
	pool->max_objsize = OBJSIZE_OF_POOL_N(nr_pools - 1);
	pool->nr_pools = nr_pools;
	int i;
	for (i = 0; i < nr_pools; i++) { /* pre-allocated, avoid race-condition */
		pool->pools[i] = alloc_new_memory_pool(i);
	}
end:
	PDEBUG("%s(%ld) = %p", __FUNCTION__, max_objsize_alloc, pool);
	if (pool) PDEBUG(" [max_objsize = %ld, nr_pools = %d]", pool->max_objsize, pool->nr_pools);
	PDEBUG_ERRNO(pool);
	return pool;
}

void vari_mempool_del(vari_mempool_t* pool)
{
	PDEBUG("%s(%p)\n", __FUNCTION__, pool);
	int i;
	for (i = 0; i < pool->nr_pools; i++) {
		printf("vari_mempool[%d]: ", i);
		memory_pool_del(pool->pools[i]);
	}
	free(pool);
}

void* vari_mempool_alloc(vari_mempool_t* pool, size_t objsize)
{
	void* p = NULL;
	memory_pool_t* pl = vari_mempool_get_pool(pool, objsize);
	if (!pl) {
		p = malloc(objsize);
		PDEBUG("%s(%ld) = %p\n", "malloc", objsize, p);
	} else {
		p = memory_pool_alloc(pl);
	}
	PDEBUG("%s(%p, %ld) = %p", __FUNCTION__, pool, objsize, p);
	PDEBUG_ERRNO(p);
	return p;
}

void vari_mempool_free(vari_mempool_t* pool, void* obj, size_t objsize) 
{
	PDEBUG("%s(%p, %p, %ld)\n", __FUNCTION__, pool, obj, objsize);
	memory_pool_t* pl = vari_mempool_get_pool(pool, objsize);
	if (!pl) {
		PDEBUG("%s(%p)\n", "free", obj);
		free(obj);
	} else {
		memory_pool_free(pl, obj);
	}
}
