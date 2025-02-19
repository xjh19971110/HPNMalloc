#ifndef TCNVMALLOC_H_
#define TCNVMALLOC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdint.h>
#include "list.h"
#include "dbg.h"
#include "dlist.h"
#include "pq.h"
#include <stdbool.h>

#define THREAD_LOCAL __attribute__ ((tls_model ("initial-exec"))) __thread
#define likely(x)           __builtin_expect(!!(x),1)
#define unlikely(x)         __builtin_expect(!!(x),0)
#define CACHE_LINE_SIZE     64
#define CACHE_ALIGN __attribute__ ((aligned (CACHE_LINE_SIZE)))
#define DEFAULT_BLOCK_CLASS (48)
#define LARGE_CLASS         (100)
#define DUMMY_CLASS         (101)
#define LARGE_OWNER         (0x5AA5)

/* configuration */
#define PAGE_SIZE 4096
#define CHUNK_DATA_SIZE (16*PAGE_SIZE)
#define CHUNK_SIZE (CHUNK_DATA_SIZE + sizeof(chunkh_t))
#define RAW_POOL_START      ((void*)((0x600000000000/CHUNK_SIZE+1)*CHUNK_SIZE))
#define ALLOC_UNIT  (1024*1024*1024)
#define WEAR_LIMIT  (50000)

#define ROUNDUP(x,n)    ((x+n-1)/n * n);

typedef struct gpool_s gpool_t;
typedef struct lheap_s lheap_t;
typedef struct chunkh_s chunkh_t;

typedef enum {
    UNINIT,
    INITED
} thread_state_t;

typedef enum {
    FORG,
    BACK,
    FULL,
    NAVA,   /* NOT AVAILABLE, ready to return to global when all clean */
} chunk_state_t;

struct chunkh_s {
    lheap_t *owner;
    chunk_state_t state;
    uint32_t size_cls;
    uint32_t blk_size;
    uint32_t blk_cnt;

    uint32_t free_mem_cnt;
    uint32_t free_tot_cnt;
    void *free_mem;

    /* FIFO free double-linked queue*/
    dlist_t dlist_head, dlist_tail;
    
    /* used to chain chunk in background list*/
    list_head list; 

    /* wear_count per allocation */
    int wear_count;

    /* total wear number */
    int wear_tot;
};

struct gpool_s {
    pthread_mutex_t lock;
    void *pool_start;
    void *pool_end;
    void *free_start;

    /* wear-aware policy is FIFO */
    list_head free_list;

    /* priority queue */
    pq_t pq;
};

struct lheap_s {
    chunkh_t *foreground[DEFAULT_BLOCK_CLASS];
    list_head background[DEFAULT_BLOCK_CLASS];

    /* returned by local thread */
    list_head free_list;
    //伪块
    chunkh_t dummy_chunk;
};

struct nvm_hardware_parameter_table
{
    // uint16_t read_delay[RAWDELAYN]; //读延时，单位ns
    // uint16_t write_delay[RAWDELAYN]; //写延时，单位ns
    double hardware_mc;        // 硬件容量大小，单位B
    double used_mc;            // 已使用容量大小，单位B
    uint64_t memory_wear_limit;  //  内存磨损上限，单位次数
    uint64_t memory_wear_remain; // 内存剩余可磨损次数
    double unit_capacity_mp;      // 单位容量内存价格
    double wear_weight;
};
void init_nvm_area_list();

bool add_nvm_area(nvm_area_t *n_area);

bool del_nvm_area(nvm_area_t *n_area);

nvm_area_t *make_init_nvm_area(double hardware_mc, double remain_mc, uint64_t memory_wear_limit, uint64_t memory_wear_remain, double unit_capacity_mp, double wear_weight);

void init_priority(nvm_area_t *n_area);

nvm_area_t *select_highest_priority_nvm_area();

void *wa_malloc(size_t size);
void *wa_realloc(void *ptr, size_t size);
void wa_free(void *ptr);


#ifdef __cplusplus
}
#endif

#endif
