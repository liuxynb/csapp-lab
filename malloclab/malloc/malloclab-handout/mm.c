/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/*Private global variables*/
static char *mem_heap;     /*Points to bytes of heap*/
static char *mem_brk;      /*Points to last byte of heap plus 1*/
static char *mem_max_addr; /*Max legal heap addr plus 1*/

static char *heap_listp;
static char *pre_listp;

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*Basic constants and macros*/
#define WSIZE 4             /*Word and header/footer size (bytes) */
#define DSIZE 8             /*Double word size*/
#define CHUNKSIZE (1 << 12) /*Extend heap by this amount (bytes)*/

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PREALLOC(x) ((!(x)) ? 0 : 2)
/*Pack a size and allocated bit into a word*/
#define PACK(size, prealloc, alloc) ((size) | (PREALLOC(prealloc)) | (alloc))

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) ((*(unsigned int *)(p)) = (val))

/*Read the size and allocated fields from address p*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREALLOC(p) (GET(p) & 0x2)

/*特别注意：    malloc 返回一个指针bp，它指向*有效载荷*的开始处*/
/*Given block ptr bp, compute address of its header and footer.*/
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/*Given block ptr bp, compute address of next and previous blocks*/
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);
inline void *set_next_prealloc(void *bp, size_t prealloc);
/*
 * mm_init - initialize the malloc package. 创建带一个初始空闲块的堆
 * 在调用 mm_malloc 或者mm _free 之前，应用必须通过调用 mm_init函数来初始化堆
 */
int mm_init(void)
{
    /*Create the initial empty heap*/
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                               /*Alignment(对齐) padding(填充)*/
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1, 1)); /*Prologue(序言) header*/
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1, 1)); /*Prologue footer*/
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1, 1));     /*Epilogue(后记/下一个） header*/
    heap_listp += (2 * WSIZE);
    pre_listp = heap_listp;
    /*Extend the empty heap with a free block of CHUNKSIZE bytes*/
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/*
 * 用一个新的空闲块扩展堆
 * 为了保持对齐，extend_heap 将请求大小向上舍入为
 * 最接近的2字(8字节的倍数，然后向内存系统请求额外的空间
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size_t prealloc;
    /*Allocate an even number(偶数！动态分配最小为双字) of words to maintain alignment*/
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    prealloc = GET_PREALLOC(HDRP(bp));
    /* Initialize free block header/footer and the epilogue header*/
    PUT(HDRP(bp), PACK(size, prealloc, 0));  /*Free block header*/
    PUT(FTRP(bp), PACK(size, prealloc, 0));  /*Free block footer*/
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1)); /*New epilogue header*/
#ifdef DEBUG
    printf("%x", bp - 4)
#endif
        /*Coalease if the previous block was free*/
        return coalesce(bp);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /*Adjusted block size */
    size_t extendsize; /*Amount to extend heap if no fit*/
    char *bp;

    /*Ignore spurious(虚假的) requests */
    if (size == 0)
        return NULL;

    /*Adjust block size to include overhead and alignment reqs.*/
    if (size <= DSIZE)
        asize = DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /*Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    /*No fit found, Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}
/*
 * mm_free - Freeing a block does nothing.
 * 释放所请求的块(bp),后使用描述的边界标记合并技术将之与邻接的空闲块合并起来。
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    size_t prealloc = GET_PREALLOC(HDRP(bp));
    PUT(HDRP(bp), PACK(size, prealloc, 0));
    PUT(FTRP(bp), PACK(size, prealloc, 0));
    coalesce(bp);
}

/*
 * coalease - Merge free blocks if possible.
 */
static void *coalesce(void *bp)
{
#ifdef DEBUG
#define CHECKHEAP(verbose)
    printf("%s\n", __func__);
    mm_checkheap(verbose);
#endif
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 1, 0));
        PUT(FTRP(bp), PACK(size, 1, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 1, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size, 1, 0));
        bp = PREV_BLKP(bp);
    }
    set_next_prealloc(bp, 0);
    pre_listp = bp;
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */

void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0)
        mm_free(ptr);
    void *newptr = NULL;
    size_t copy_size = 0;
    if ((newptr = mm_malloc(size)) == NULL)
        return NULL;
    size = GET_SIZE(HDRP(ptr));
    copy_size = GET_SIZE(HDRP(ptr));
    if (size < copy_size)
        copy_size = size;
    memcpy(newptr, ptr, copy_size - WSIZE);
    mm_free(ptr);
    return newptr;
}
/*
 * 假如给该块分配的空间大于（实际所需空间+DSIZE），则需要对未利用的部分进行分割和放置。
 * asize 为实际所需要的空间(对齐后(Adjusted))。
 */
static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));

    if ((size - asize) >= DSIZE)
    {
        PUT(HDRP(bp), PACK(asize, 1, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(size - asize, 1, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size - asize, 1, 0));
        set_next_prealloc(bp, 0);
    }
    else
    {
        PUT(HDRP(bp), PACK(size, 1, 1));
        set_next_prealloc(bp, 1);
    }
    pre_listp = bp;
}
/*
 * 指示该函数或方法分配预先分配的内存。
 */
inline void *set_next_prealloc(void *bp, size_t prealloc)
{
    size_t size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
    size_t alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(size, prealloc, alloc));
}
/*
 * 寻找匹配的空闲块。
 */
static void *find_fit(size_t asize)
{
    char *bp = pre_listp;
    size_t alloc;
    size_t size;
    while (GET_SIZE(HDRP(NEXT_BLKP(bp))) > 0)
    {
        bp = NEXT_BLKP(bp);
        alloc = GET_ALLOC(HDRP(bp));
        if (alloc)
            continue;
        size = GET_SIZE(HDRP(bp));
        if (size < asize)
            continue;
        return bp;
    }
    bp = heap_listp;
    while (bp != pre_listp)
    {
        bp = NEXT_BLKP(bp);
        alloc = GET_ALLOC(HDRP(bp));
        if (alloc)
            continue;
        size = GET_SIZE(HDRP(bp));
        if (size < asize)
            continue;
        return bp;
    }
    return NULL;
}