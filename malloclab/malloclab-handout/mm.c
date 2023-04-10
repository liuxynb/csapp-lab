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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*Basic constants and macros*/
#define WordSize 4
#define DWordSize 8 /*double word size(bytes)*/
#define InitChunkSize (1 << 6)
#define ChunkSize (1 << 12) /*extend heap by this amount bytes*/

#define MAXNUMBER 16

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) > (y) ? (y) : (x))
/*Pack a size and allocated bit into a word*/

/*The header of the chunk(块) -> [the size of the chunk(29)][padding bit(2)][allocated?(1)] */
#define PACK(size, alloc) ((size) | (alloc))

/*Read and write a word at address p*/
// Get the info of the address (1 byte (32 bits))
#define GET(p) (*(unsigned int *)(p))
// Assign the content of a word to the address
#define PUT(p, value) (*(unsigned int *)(p) = (value))

/*Read the size and allocated fields from address p  */
// Get the chunk size
#define GET_SIZE(p) (GET(p) & ~0x7) /*(0111) So get the higher 29 bits*/
// Get the last bit to check if allocated.
#define GET_ALLOC(p) (GET(p) & 0x1)

// bp 代表块指针，位置是头部一个字的地址，也就是有效载荷的开头地址
/*Given block ptr bp ,compute address of its header and footer*/
// bp - one word = the address of the head
#define HDP(bp) ((char *)(bp)-WordSize)
// bp + (the chunk size - the size of head and foot) = the address of the foot right now
#define FTP(bp) ((char *)(bp) + GET_SIZE(HDP(bp)) - DWordSize)

/*Given block ptr bp, compute address(bp) of next and previous blocks*/
// bp + 当前块大小（由头部得到） =  下一个地址
#define NEXT_BLOCKP(bp) ((char *)(bp) + GET_SIZE(HDP(bp)))
// bp - 上一个块大小（由脚部得到） = 上一个块的地址
#define PREV_BLOCKP(bp) ((char *)(bp)-GET_SIZE(FTP(bp)))

// 自由块的前一个和后一个条目的地址
#define PRED_PTR(bp) ((char *)(bp)) // pred：前任
#define SUCC_PTR(bp) ((char *)(bp) + WordSize)

// 第二维中的前一个块地址和后一个块地址
#define PRED(bp) (*(char **)(bp))
#define SUCC(bp) (*(char **)(SUCC_PTR(bp)))

//Put the address of ptr into the pointer p
#define SET_PTR(p,ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

//获取和设置列表中的值
#define GET_FREE_LIST_PTR(i) (*(free_lists+i))
#define SET_FREE_LIST_PTR(i, bp) (*(free_lists+i)=bp)

/*Glabal varibles*/
static char *heap_listp = 0; /*Pointer to first block*/
static char** free_lists; /*Store the free 2-demation arrays*/

/*Function prototypes for internal helper routines*/
static void *extend_heap(size_t size);
static void *coalescs(void *bp);
static void *place(void *bp,size_t asize);
static void *add(void *bp,size_t size);
static void *delete(void *bp);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((long)(free_lists = mem_sbrk(MAXNUMBER*sizeof(char*)))==-1)
    {
        return -1;
    }
    //initial the free lists:
    for (int i = 0; i < MAXNUMBER; i++)
    {
        SET_FREE_LIST_PTR(i,NULL);        /* code */
    }
    /*创建一个空堆*/
    //申请4个子的块空间
    if((long)(heap_listp = mem_sbrk(4*WordSize))==-1)
    {
        return -1;
    }
    PUT(heap_listp,0);/*对齐填充*/
    PUT(heap_listp+(1*WordSize),PACK(DWordSize,1));/*序言块头部*/
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
