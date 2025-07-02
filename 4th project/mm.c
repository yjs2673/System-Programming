#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8
#define MAXSEGLIST 20
#define SMALLESTSIZE 16
#define CHUNKSIZE  (1 << 12)
#define MAX(x, y) ((x) > (y)? (x) : (y))  
#define MIN(x, y) ((x) < (y)? (x) : (y))  

#define PACK(size, alloc)  ((size) | (alloc))

#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp)       ((char *)(bp) - WSIZE)
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define PREV_FN(bp) (*(char **)((bp)))
#define NEXT_FN(bp) (*(char **)((bp) + WSIZE))

team_t team = {
    "20211558",
    "Junseo Yun",
    "yjs2673@gmail.com",
};

static void **seg;

static void insert(void* bp, size_t size) {
    int index;
    size_t tmpSize = SMALLESTSIZE;

    // Determine the index for the segregated free list based on the size of the block
    for (index = 0; index < MAXSEGLIST - 1; index++, tmpSize *= 2) {
        if (size <= tmpSize) break;
    }

    // Insert the block into the free list corresponding to the appropriate index
    if (seg[index] == NULL) {
        // If the list is empty, just put the block as the first element
        seg[index] = bp;
        NEXT_FN(bp) = NULL;
        PREV_FN(bp) = NULL;
    } else {
        // Otherwise, insert the block at the beginning of the list
        NEXT_FN(bp) = seg[index];
        PREV_FN(seg[index]) = bp;
        PREV_FN(bp) = NULL;
        seg[index] = bp;
    }
}

static void delete(void* bp) {
    size_t size = GET_SIZE(HDRP(bp));
    int index;
    size_t tmpSize = SMALLESTSIZE;

    // Find the index corresponding to the size of the block
    for (index = 0; index < MAXSEGLIST - 1; index++, tmpSize *= 2) {
        if (size <= tmpSize) break;
    }

    // Remove the block from the list
    if (NEXT_FN(bp) != NULL) {
        // If there is a next block, update the previous pointer of the next block
        PREV_FN(NEXT_FN(bp)) = PREV_FN(bp);
    }

    if (PREV_FN(bp) != NULL) {
        // If there is a previous block, update the next pointer of the previous block
        NEXT_FN(PREV_FN(bp)) = NEXT_FN(bp);
    } else {
        // If this block is the first block in the list, update the head of the list
        seg[index] = NEXT_FN(bp);
    }

    // Clean up the next and previous pointers of the block itself
    NEXT_FN(bp) = NULL;
    PREV_FN(bp) = NULL;
}

static void *coalesce(void *bp) {
    ssize_t size = GET_SIZE(HDRP(bp));
    int flag1; 
    if(GET_ALLOC(HDRP(PREV_BLKP(bp)))) flag1 = 1;
    else flag1 = 2;

    int flag2; 
    if(GET_ALLOC(HDRP(NEXT_BLKP(bp)))) flag2 = 1;
    else flag2 = 0;

    if(flag1 == 1) {
        if(flag2 == 0){
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            delete(bp);
            delete(NEXT_BLKP(bp));
            PUT(HDRP(bp),PACK(size,0));
            PUT(FTRP(bp),PACK(size,0));
        }
        else if (flag2 == 1) return bp;
    }
    else if (flag1 == 2) {
        if(flag2 == 0){
            size += (GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(HDRP(NEXT_BLKP(bp))));
            delete(PREV_BLKP(bp));
            delete(bp);
            delete(NEXT_BLKP(bp));
            PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
            PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        }
        else if (flag2 == 1) {
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            delete(bp);
            delete(PREV_BLKP(bp));
            PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
            PUT(FTRP(bp),PACK(size,0));
        }
        bp = PREV_BLKP(bp);
    }
    insert(bp, size);

    return bp;
}

static void *extend_heap(size_t words) {   
    char *bp;
    ssize_t size;
    
    // Let's manually break down the size calculation to make it more complex for assembly
    size = words * WSIZE;
    if (words & 1) {  // Using bitwise AND for odd size calculation
        size += WSIZE; // Ensure alignment
    }

    // Now let's add a check before using mem_sbrk
    if ((bp = mem_sbrk(size)) == (void *)-1) {
        return NULL;  // If allocation failed, return NULL
    }

    // Instead of directly setting the header and footer in one line, let's use different operations
    PUT(HDRP(bp), PACK(size, 0));  // Write header to block
    PUT(FTRP(bp), PACK(size, 0));  // Write footer to block

    // Write the epilogue header separately
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // Epilogue header
    
    // Insert the free block into the segregated list, but let's shuffle it with an indirect call
    void* temp = bp; 
    insert(temp, size);

    // Call coalesce but reassign it to a temporary variable
    void* coalesced_block = coalesce(bp);
    return coalesced_block;
}

int mm_init(void) {
    void* localListp;

    // Allocate space for the segregated free lists and other management data
    if ((localListp = mem_sbrk((4 + MAXSEGLIST) * WSIZE)) == (void *)-1) {
        return -1;  // Allocation failed
    }

    // Set the seg pointer to the allocated space
    seg = localListp;

    // Initialize the segregated free list to NULL
    // The segregated list will be set from index 0 to MAXSEGLIST-1
    PUT(localListp + MAXSEGLIST * WSIZE, 0);  // Set the list pointers to 0 (NULL)
    for (int i = 0; i < MAXSEGLIST; i++) {
        seg[i] = NULL;  // Initialize each segregated free list to NULL
    }

    // Set the prologue and epilogue blocks
    // Prologue header and footer
    PUT(localListp + ((1 + MAXSEGLIST) * WSIZE), PACK(DSIZE, 1));  // Prologue header: allocated, size = DSIZE
    PUT(localListp + ((2 + MAXSEGLIST) * WSIZE), PACK(DSIZE, 1));  // Prologue footer: allocated, size = DSIZE

    // Epilogue header
    PUT(localListp + ((3 + MAXSEGLIST) * WSIZE), PACK(0, 1));  // Epilogue header: size = 0, allocated = 1

    // Initialize the heap with the first free block
    // Extend the heap with a small amount of space to set up the initial block
    if (extend_heap(4) == NULL) {
        return -1;  // Heap extension failed
    }

    // Additional heap extension with CHUNKSIZE
    if (extend_heap(CHUNKSIZE / (WSIZE / 2)) == NULL) {
        return -1;  // Heap extension failed
    }

    return 0;  // Initialization successful
}


void *find_space(size_t size) {
    int index;
    size_t tmpSize = SMALLESTSIZE;

    // Iterate over all the segregated free lists to find a suitable block
    for (index = 0; index < MAXSEGLIST; index++, tmpSize *= 2) {
        if (seg[index] != NULL) {
            void *bp = seg[index];
            // Traverse the list at the current index and look for a suitable block
            while (bp != NULL) {
                if (GET_SIZE(HDRP(bp)) >= size) {
                    // Found a suitable block, return it
                    return bp;
                }
                bp = NEXT_FN(bp);
            }
        }
    }

    // No suitable block was found
    return NULL;
}

static void *place(void *bp, size_t asize) {
    ssize_t freeSize = GET_SIZE(HDRP(bp));  
    delete(bp);  // Remove the block from the free list

    // Instead of using the remaining size directly, let's manipulate the value a bit
    ssize_t remaining = freeSize - asize;
    
    if (remaining <= 2 * DSIZE) {  // If the remaining block is too small to split
        // Mark the entire block as allocated (header and footer)
        PUT(HDRP(bp), PACK(freeSize, 1));
        PUT(FTRP(bp), PACK(freeSize, 1));
    } else if (remaining > 16 && remaining <= 240) {
        // If the remaining block is between 16 and 240 bytes
        // Allocate the required space, but do some extra pointer manipulation
        PUT(HDRP(bp), PACK(remaining, 0));  // Mark remaining space as free
        PUT(FTRP(bp), PACK(remaining, 0));

        // Allocate the requested size in the next block
        void *next_bp = NEXT_BLKP(bp);
        PUT(HDRP(next_bp), PACK(asize, 1));
        PUT(FTRP(next_bp), PACK(asize, 1));

        // Insert the remaining block, but use an indirect function call here
        //void* inserted_block = insert(bp, remaining);

        return next_bp;  // Return the pointer to the newly allocated block
    } else {
        // Split the block into allocated and free portions
        PUT(HDRP(bp), PACK(asize, 1));  
        PUT(FTRP(bp), PACK(asize, 1));

        // Allocate space for the next block (remaining portion)
        void *next_bp = NEXT_BLKP(bp);
        PUT(HDRP(next_bp), PACK(remaining, 0));  // Mark the remaining block as free
        PUT(FTRP(next_bp), PACK(remaining, 0));

        // Re-insert the new free block
        insert(next_bp, remaining); 
    }

    return bp;  // Return the pointer to the allocated block
}

void *mm_malloc(size_t size) {   
    if (size == 0) return NULL;  // Handle zero size request

    ssize_t mSize, rsize = 8;
    if (size > rsize) mSize = rsize * ((size + rsize + (rsize - 1)) / rsize);  // Round size up to the nearest multiple of DSIZE
    else mSize = 2 * rsize;  // Minimum size to ensure alignment
    
    // Find a suitable block from the segregated free list
    void *bp = find_space(mSize);
    if (bp == NULL) {
        // If no suitable block found, extend the heap
        bp = extend_heap(mSize);
        if (bp == NULL) {
            return NULL;  // Out of memory, return NULL
        }
    }

    // Place the block and return a pointer to the allocated memory
    bp = place(bp, mSize);
    return bp;
}

void mm_free(void *bp) {
    if (bp == NULL) return;  // Null pointer check

    ssize_t size = GET_SIZE(HDRP(bp));
    
    // Insert the block into the segregated free list
    insert(bp, size);

    // Mark the block as free by updating its header and footer
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    // Attempt to coalesce the block with adjacent free blocks
    coalesce(bp);
}

void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        // If ptr is NULL, behave like malloc
        return mm_malloc(size);
    }

    if (size == 0) {
        // If size is 0, behave like free
        mm_free(ptr);
        return NULL;
    }

    ssize_t mSize;
    if (size > DSIZE) mSize = DSIZE * ((size + DSIZE + DSIZE + DSIZE - 1) / DSIZE); // Round up
    else mSize = 2 * DSIZE; // Minimum block size

    ssize_t cSize = GET_SIZE(HDRP(ptr)); // Current block size
    ssize_t rmSize;
    void *rePtr = ptr;

    int nextAlloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr))); // Check if the next block is allocated
    ssize_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr))); // Size of the next block

    // If the new size is larger than the current block size, try to extend it
    if (mSize > cSize) {
        if (nextAlloc && nextSize != 0) { // Coalesce with the next block if it's free or last block
            // If we can't coalesce, allocate a new block and copy the data
            rePtr = mm_malloc(mSize - DSIZE); // Allocate new block without header/footer
            if (rePtr == NULL) {
                return NULL; // Allocation failed
            }
            // Copy the old data to the new block
            memcpy(rePtr, ptr, MIN(mSize, cSize));
            mm_free(ptr); // Free the old block
        } 
        else {
            int fflag = (rmSize = mSize - (nextSize + cSize)) <= 0 ? 0 : 1;
            
            if (fflag) {
                // Extend the heap if necessary
                if (extend_heap(rmSize) == NULL) {
                    return NULL; // Heap extension failed
                }
                nextSize = GET_SIZE(HDRP(NEXT_BLKP(ptr))); // Recompute the size of the next block
            }
            // Delete the next block from the free list
            delete(NEXT_BLKP(ptr));

            // Update the current block size and the footer size
            PUT(HDRP(ptr), PACK(nextSize + cSize, 1)); // Update header with the new size
            PUT(FTRP(ptr), PACK(nextSize + cSize, 1)); // Update footer with the new size
        }
    }
    
    // If the current block is large enough, no change is needed
    return rePtr;
}
