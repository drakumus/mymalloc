/*
 * mm-naive.c - The least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by allocating a
 * new page as needed.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused.
 *
 * The heap check and free check always succeeds, because the
 * allocator doesn't depend on any of the old data.
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


typedef struct {
  size_t size;
  char allocated;
} block_header;

typedef struct {
  size_t size;
  char allocated;
} block_footer;

typedef struct {
  void* start_pointer; //points to start of next page

} page_footer;

typedef struct {
  void* start_pointer; //points to end of previous page
} page_header;
/* always use 16-byte alignment */
#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

/* rounds up to the nearest multiple of mem_pagesize() */
#define PAGE_ALIGN(size) (((size) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

#define HDRP(bp) ((char *) (bp) - sizeof(block_header)) //find start of data
#define GET_SIZE(p) ((block_header *) (p))->size //extract size from a header struct
#define GET_ALLOC(p) ((block_header *) (p))->allocated //extract allocated from header struct
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp))) //add payload size to header size to reach next block
#define CHUNK_SIZE (1 << 14)
#define CHUNK_ALLIGN(size) (((size)+(CHUNK_SIZE-1)) & ~(CHUNK_SIZE-1))
#define PAGE_BYTES(num) (num * mem_pagesize()) 

#define GET_PREVIOUS_PAGE(p) ((page_header *) (p))->start_pointer
#define GET_NEXT_PAGE(p) ((page_footer *) (p))->start_pointer

void set_allocated(void *b, size_t size);
void extend(size_t s);

//bp = block payload which is where the stored data starts

void *current_avail = NULL;
int current_avail_size = 0;

void* first_bp;
void* current_bp;

size_t OVERHEAD = sizeof(block_header);

/* 
 * mm_init - initialize the malloc package.
 */
// LAYOUT: [PAGE START] [...] [16 Byte Blocks] [...] [PAGE END]
int mm_init(void)
{
  //current_avail = NULL;
  //current_avail_size = 0;
  current_avail_size = PAGE_BYTES(2); //set available size to 2 pages worth of bytes
  first_bp = mem_map(PAGE_BYTES(2)); //get start pointer

  //set up page footer
  GET_NEXT_PAGE(first_bp + current_avail_size - 16) = -1; //gets last 16 byte block
  current_avail_size -= 16; //remove those 16 bytes from writable bytes
  
  //set up page header
  GET_PREVIOUS_PAGE(first_bp) = 0; //for first page set previous page pointer to 0
  first_bp += ALIGN(sizeof(page_header)); //set the start of the first block pointer to after the page_header

  current_avail_size -= 16; //subtract the page header size from the total available size

  //set up first block header. These values for block header are unique to block header
  GET_SIZE(HDRP(first_bp)) = 0; //set the block header byte count to 0
  GET_ALLOC(HDRP(first_bp)) = 1; //set the block header set allocated to 1

  current_avail_size -= 16; //subtract the block header from available number of bytes

  

  return 0;
}

/* 
 * mm_malloc - Allocate a block by using bytes from current_avail,
 *     grabbing a new page if necessary.
 */
// void *mm_malloc(size_t size)
// {
//   int newsize = ALIGN(size);
//   void *p;
  
//   if (current_avail_size < newsize) {
//     current_avail_size = PAGE_ALIGN(newsize);
//     current_avail = mem_map(current_avail_size);
//     if (current_avail == NULL)
//       return NULL;
//   }

//   p = current_avail;
//   current_avail += newsize;
//   current_avail_size -= newsize;
  
//   return p;
// }

void *mm_malloc(size_t size)
{
  int new_size = ALIGN(size + OVERHEAD); //sets up an aligned block that includes the header overhead
  void *bp = first_bp; //start the block pointer at first_bp to loop through

  while (GET_NEXT_PAGE(bp) != -1) //keep looping until page footer is hit.
  {
    if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= new_size)) //write when block isn't allocated and enough size is available
    {
      set_allocated(bp, new_size); //pass the block pointer and size
      return bp;
    }
    bp = NEXT_BLKP(bp); //step to next bp
  }

  extend(new_size); //extend (check if you need to extend here too) to next page if needed
  set_allocated(first_bp, new_size); //allocate after extend since this case is only hit 

  return bp;
}

void set_allocated(void *bp, size_t size)
{
  size_t extra_size = GET_SIZE(HDRP(bp)) - size; //gets how much room is left in newly allocated space

  if (extra_size > ALIGN(1 + OVERHEAD)) //case for if there's extra room
  {
    GET_SIZE(HDRP(bp)) = size; //set size to size
    GET_SIZE(HDRP(NEXT_BLKP(bp))) = extra_size; //set next block to leftover size
    GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 0; //set next block to unallocated
  }

  GET_ALLOC(HDRP(bp)) = 1; //case for if the block fits perfectly. allocation bit is set. size does not change
}

void extend(size_t new_size)
{
  //size_t chunk_size = PAGE_ALIGN(new_size);
  //void *bp = mem_map(chunk_size);  //need int or something to track bits used here
                                   //and a current bp tracker.
  
  if(current_avail_size < new_size)
  {
    current_bp = mem_map(PAGE_BYTES(2));
    current_avail_size = PAGE_BYTES(2); //reset available size

    GET_NEXT_PAGE(PAGE_BYTES(2)-sizeof(page_footer)) = current_bp; //track next page here
    GET_PREVIOUS_PAGE(current_bp) = first_bp - sizeof(page_header); //set pointer to previous page

    current_avail_size -= sizeof(page_header); //update available size

    first_bp = current_bp + sizeof(page_header); //move first pointer to after page header
    GET_SIZE(HDRP(first_bp)) = 0; //set up prolog block
    GET_ALLOC(HDRP(first_bp)) = 1; //

    current_avail_size -= sizeof(block_header); //update available size
  }
  //GET_SIZE(HDRP(bp)) = chunk_size; //
  //GET_ALLOC(HDRP(bp)) = 0;

  //GET_SIZE(HDRP(NEXT_BLKP(bp))) = 0;
  //GET_ALLOC(HDRP(NEXT_BLKP(bp))) = 1;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
  GET_ALLOC(HDRP(ptr)) = 0;
}

/*
 * mm_check - Check whether the heap is ok, so that mm_malloc()
 *            and proper mm_free() calls won't crash.
 */
int mm_check()
{
  return 1;
}

/*
 * mm_check - Check whether freeing the given `p`, which means that
 *            calling mm_free(p) leaves the heap in an ok state.
 */
int mm_can_free(void *p)
{
  if(GET_ALLOC(HDRP(p)) == 1)
    return 1;
  return 0;
}
