// ==============================================================================
/**
 * pb-alloc.c
 *
 * A _pointer-bumping_ heap allocator.  This allocator *does not re-use* freed
 * blocks.  It uses _pointer bumping_ to expand the heap with each allocation.
 **/
// ==============================================================================



// ==============================================================================
// INCLUDES

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include "safeio.h"
// ==============================================================================



// ==============================================================================
// MACRO CONSTANTS AND FUNCTIONS

/** The system's page size. */
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

/**
 * Macros to easily calculate the number of bytes for larger scales (e.g., kilo,
 * mega, gigabytes).
 */
#define KB(size)  ((size_t)size * 1024)
#define MB(size)  (KB(size) * 1024)
#define GB(size)  (MB(size) * 1024)

/** The virtual address space reserved for the heap. */
#define HEAP_SIZE GB(2)
// ==============================================================================


// ==============================================================================
// TYPES AND STRUCTURES

/** A header for each block's metadata. */
typedef struct header {

  /** The size of the useful portion of the block, in bytes. */
  size_t size;
  
} header_s;
// ==============================================================================



// ==============================================================================
// GLOBALS

/** The address of the next available byte in the heap region. */
static intptr_t free_addr  = 0;

/** The beginning of the heap. */
static intptr_t start_addr = 0;

/** The end of the heap. */
static intptr_t end_addr   = 0;
// ==============================================================================



// ==============================================================================
/**
 * The initialization method.  If this is the first use of the heap, initialize it.
 */

void init () {

  // Only do anything if there is no heap region (i.e., first time called).
  if (start_addr == 0) {

    DEBUG("Trying to initialize");
    
    // Allocate virtual address space in which the heap will reside. Make it
    // un-shared and not backed by any file (_anonymous_ space).  A failure to
    // map this space is fatal.
    void* heap = mmap(NULL,
		      HEAP_SIZE,
		      PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS,
		      -1,
		      0);
    if (heap == MAP_FAILED) {
      ERROR("Could not mmap() heap region");
    }

    // Hold onto the boundaries of the heap as a whole.
    start_addr = (intptr_t)heap;
    end_addr   = start_addr + HEAP_SIZE;
    free_addr  = start_addr;

    // DEBUG: Emit a message to indicate that this allocator is being called.
    DEBUG("bp-alloc initialized");

  }

} // init ()
// ==============================================================================


// ==============================================================================
/**
 * Allocate and return `size` bytes of heap space.  Expand into the heap region
 * via _pointer bumping_.
 *
 * \param size The number of bytes to allocate.

 * \return A pointer to the allocated block, if successful; `NULL` if
 *         unsuccessful.
 */
void* malloc (size_t size) {

  init(); //initialize the heap the FIRST time malloc gets called	

  if (size == 0) { //if requested block size is zeero, return null
    return NULL;
  }

  size_t    total_size = size + sizeof(header_s); //header size has to be accounted for in the total size of the allocated memory
  
  size_t    pad = 16 - ( (free_addr + sizeof(header_s)) % 16 ); //padding to be added before header and after free_addr for double world alignment of block_ptr
  pad = pad % 16; //already aligned if free_addr + header_size == 16, so no padding
  header_s* header_ptr = (header_s*) (free_addr + pad); //pointer to header of the block replaced with address to next available byte in the heap
  void*     block_ptr  = (void*)(free_addr + pad + sizeof(header_s)); //block_ptr becomes address to header, shifted right by the header size
  intptr_t new_free_addr = free_addr + pad + total_size; //next available free address may be shifted to the right by (block_size + header_size)
  if (new_free_addr > end_addr) { //if next possibly free address goes out of the stack's bounds, return Null
    return NULL;
  } else { //else, the address of the next available block (free-addr) updates
    free_addr = new_free_addr;
  }
  header_ptr->size = size; //(*header_ptr).size = size
                          //update the header's (pointed to by header_ptr) size property to the requested block when calling malloc
  return block_ptr; //returns pointer to block (not header)
} // malloc()
// ==============================================================================



// ==============================================================================
/**
 * Deallocate a given block on the heap.  Add the given block (if any) to the
 * free list.
 *
 * \param ptr A pointer to the block to be deallocated.
 */
void free (void* ptr) {

  DEBUG("free(): ", (intptr_t)ptr);

} // free()
// ==============================================================================



// ==============================================================================
/**
 * Allocate a block of `nmemb * size` bytes on the heap, zeroing its contents.
 *
 * \param nmemb The number of elements in the new block.
 * \param size  The size, in bytes, of each of the `nmemb` elements.
 * \return      A pointer to the newly allocated and zeroed block, if successful;
 *              `NULL` if unsuccessful.
 */
void* calloc (size_t nmemb, size_t size) {

  // Allocate a block of the requested size.
  size_t block_size = nmemb * size;
  void*  block_ptr  = malloc(block_size);

  // If the allocation succeeded, clear the entire block.
  if (block_ptr != NULL) {
    memset(block_ptr, 0, block_size);
  }

  return block_ptr;
  
} // calloc ()
// ==============================================================================



// ==============================================================================
/**
 * Update the given block at `ptr` to take on the given `size`.  Here, if `size`
 * fits within the given block, then the block is returned unchanged.  If the
 * `size` is an increase for the block, then a new and larger block is
 * allocated, and the data from the old block is copied, the old block freed,
 * and the new block returned.
 *
 * \param ptr  The block to be assigned a new size.
 * \param size The new size that the block should assume.
 * \return     A pointer to the resultant block, which may be `ptr` itself, or
 *             may be a newly allocated block.
 */
void* realloc (void* ptr, size_t size) {

  if (ptr == NULL) { //call malloc normally to allocate new block if the pointer is Null
    return malloc(size);
  }
  if (size == 0) { //to resize to 0, free the block completely. I.e. call free on the given pointer
    free(ptr);
    return NULL;
  }

  header_s* old_header = (header_s*)((intptr_t)ptr - sizeof(header_s)); // old block's header pointer = point_to_block - header_size
  size_t    old_size   = old_header->size; // retrieve old_block_size from its header
                                          // same as: size_t old_size = (*old_header).size

  if (size <= old_size) { //if desired size (for the block) is ≤ old_size, then can use old pointer
    return ptr;
  }
  void* new_ptr = malloc(size); //"Else", allocate new memory block of necessary size
  if (new_ptr != NULL) { //when heap is full or when size==0...
    memcpy(new_ptr, ptr, old_size); //... copy old_block at ptr (with old_size) into block at new_ptr
    free(ptr); //...and free the old_block fat ptr
  }
  return new_ptr; //pointer to our new_block
  
} // realloc()
// ==============================================================================



#if defined (ALLOC_MAIN)
// ==============================================================================
/**
 * The entry point if this code is compiled as a standalone program for testing
 * purposes.
 */
int main () {

  // Allocate a few blocks, then free them.
  void* x = malloc(16);
  void* y = malloc(64);
  void* z = malloc(32);

  free(z);
  free(y);
  free(x);

  return 0;
  
} // main()
// ==============================================================================
#endif
