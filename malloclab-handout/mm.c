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
#include "mm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#define GET(p) (*(uint32_t*)(p))
#define PUT(p, val) ((*(uint32_t*)(p)) = (val))

#define PACK(size, alloc) ((size) | (alloc))

#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_SIZE(p) (GET(p) & ~0x7)

#define HDRP(bp) ((char*)(bp)-WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE((char*)(bp)-WSIZE))
#define PREV_BLKP(bp) ((char*)(bp)-GET_SIZE((char*)(bp)-DSIZE))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

uint32_t* heap_listp;

static void* coalesce(void* bp);
static void* extend_heap(size_t size);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  uint32_t* heap_listp = (uint32_t*)mem_sbrk(ALIGN(4 * WSIZE));

  PUT(heap_listp, 0);                                // Padding
  PUT(heap_listp + (1 * WSIZE), PACK(2 * WSIZE, 1)); // Prologue header
  PUT(heap_listp + (2 * WSIZE), PACK(2 * WSIZE, 1)); // Prologue footer
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));         // Epilogue
  heap_listp += (2 * WSIZE);                         // Point to first block now

  if (extend_heap(ALIGN(CHUNKSIZE)) == (void*)-1) {
    return -1;
  }

  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size) {
  int newsize = ALIGN(size + SIZE_T_SIZE);
  void* p = mem_sbrk(newsize);
  if (p == (void*)-1)
    return NULL;
  else {
    *(size_t*)p = size;
    return (void*)((char*)p + SIZE_T_SIZE);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* ptr) {
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size) {
  void* oldptr = ptr;
  void* newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL)
    return NULL;
  copySize = *(size_t*)((char*)oldptr - SIZE_T_SIZE);
  if (size < copySize)
    copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

static void* extend_heap(size_t size) {
  void* bp;
  size_t asize = ALIGN(size + SIZE_T_SIZE);

  if ((bp = mem_sbrk(asize)) == (void*)-1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(asize, 0));
  PUT(FTRP(bp), PACK(asize, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); // Set new epilogue

  return coalesce(bp);
}

static void* coalesce(void* bp) {
  int prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  int next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (!prev_alloc && !next_alloc) {
    return bp;
  }

  else if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
  }

  else if (!prev_alloc && next_alloc) {
    size += GET_SIZE(FTRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  else {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(FTRP(PREV_BLKP(bp)));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }

  return bp;
}
