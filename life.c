/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <pthread.h>

#define NUM_THREADS 8

#define SWAP_BOARDS(b1, b2)  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD(__board, __i, __j)  (__board[(__i) + LDA*(__j)])


/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/
struct thread_argument_t {
    char *outboard;
    char *inboard;
    int nrows;
    int ncols;
    int ith_slice;
};



void* worker_fuction_by_rows(void *args) {
	struct thread_argument_t* arg = (struct thread_argument_t*)arg;
	int ith_slice = arg->ith_slice;
	int slice_size = arg->nrows / NUM_THREADS;
	int nrows = arg->nrows;
	int ncols = arg->ncols;
	char *outboard = arg->outboard;
	char *inboard = arg->inboard;
	for (int i = ith_slice * slice_size; i < (ith_slice + 1) * slice_size; ++i){
		for (int j = 0; j < arg->nrows; ++j) {
				const int inorth = mod(i - 1, nrows);
                const int isouth = mod(i + 1, nrows);
                const int jwest = mod(j - 1, ncols);
                const int jeast = mod(j + 1, ncols);

                const char neighbor_count =
                        BOARD(inboard, inorth, jwest) +
                        BOARD(inboard, inorth, j) +
                        BOARD(inboard, inorth, jeast) +
                        BOARD(inboard, i, jwest) +
                        BOARD(inboard, i, jeast) +
                        BOARD(inboard, isouth, jwest) +
                        BOARD(inboard, isouth, j) +
                        BOARD(inboard, isouth, jeast);

            BOARD(outboard, i, j) = alivep(neighbor_count, BOARD(inboard, i, j));

		}
	}
	return NULL;
}

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char *
game_of_life(char *outboard,
             char *inboard,
             const int nrows,
             const int ncols,
             const int gens_max) {
    if (nrows < 32 && ncols < 32)
        return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
    else {
        pthread_t worker_threads[NUM_THREADS];
        return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max, worker_threads);
    }
}


char * parallel_game_of_life(char *outboard,
                      char *inboard,
                      const int nrows,
                      const int ncols,
                      const int gens_max,
                      pthread_t *worker_threads) {
	// barrier var
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    const int LDA = nrows;
    for (int curgen = 0; curgen < gens_max; ++curgen) {

        /*Thought 1: slice the board by rows, i.e. horizontally*/
        // initialize the thread arguments
        // struct thread_argument_t *args = malloc(sizeof(struct thread_argument_t));
    	struct thread_argument_t args[NUM_THREADS];

        for (int i = 0; i < NUM_THREADS; ++i) {
	        args[i].outboard = outboard;
	        args[i].inboard = inboard;
	        args[i].nrows = nrows;
	        args[i].ncols = ncols;	
        	args[i].ith_slice = i;
            pthread_create(&worker_threads[i], NULL, worker_fuction_by_rows, &args[i]);
        }
        // barrier that makes sure every worker thread has done their slice of work
        pthread_barrier_wait(&barrier); 
        // can swap the board now
        SWAP_BOARDS(outboard, inboard);
    }
    return inboard;
}


char*
sequential_game_of_life(char *outboard,
                        char *inboard,
                        const int nrows,
                        const int ncols,
                        const int gens_max) {
    /* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */
    const int LDA = nrows;
    int curgen, i, j;

    for (curgen = 0; curgen < gens_max; curgen++) {
        /* HINT: you'll be parallelizing these loop(s) by doing a
           geometric decomposition of the output */
        for (i = 0; i < nrows; i++) {
            for (j = 0; j < ncols; j++) {
                const int inorth = mod(i - 1, nrows);
                const int isouth = mod(i + 1, nrows);
                const int jwest = mod(j - 1, ncols);
                const int jeast = mod(j + 1, ncols);

                const char neighbor_count =
                        BOARD (inboard, inorth, jwest) +
                        BOARD (inboard, inorth, j) +
                        BOARD (inboard, inorth, jeast) +
                        BOARD (inboard, i, jwest) +
                        BOARD (inboard, i, jeast) +
                        BOARD (inboard, isouth, jwest) +
                        BOARD (inboard, isouth, j) +
                        BOARD (inboard, isouth, jeast);

                BOARD(outboard, i, j) = alivep(neighbor_count, BOARD (inboard, i, j));

            }
        }

        SWAP_BOARDS(outboard, inboard);
    }
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}


