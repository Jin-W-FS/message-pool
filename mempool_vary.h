#ifndef VARIABLE_SIZED_MEMPOOL_H
#define VARIABLE_SIZED_MEMPOOL_H

struct vari_mempool;
typedef struct vari_mempool vari_mempool_t;

struct vari_mempool_cfg {
	size_t max_objsize_alloc;
};

vari_mempool_t* vari_mempool_new(struct vari_mempool_cfg* cfg);
void vari_mempool_del(vari_mempool_t* pool);

void* vari_mempool_alloc(vari_mempool_t* pool, size_t objsize);
void vari_mempool_free(vari_mempool_t* pool, void* obj, size_t objsize);

#endif

