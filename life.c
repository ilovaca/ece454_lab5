/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <pthread.h>



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

void do_cell(char *outboard, char *inboard, int i, int j, const int LDA);
void board_init(char *board, int size);

void* worker_fuction_by_rows(void *args) {
  struct thread_argument_t *arg = (struct thread_argument_t*)args;
  int ith_slice = arg->ith_slice;
  int slice_size = arg->nrows / NUM_THREADS;
  int nrows = arg->nrows;
  int ncols = arg->ncols;
  char *outboard = arg->outboard;
  char *inboard = arg->inboard;

  for (int i = ith_slice * slice_size; i < (ith_slice + 1) * slice_size; ++i){

    for (int j = 0; j < arg->nrows; ++j) {
	do_cell(outboard, inboard, i, j, nrows);
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
    if (nrows < 32) {
        return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
    }
    else if (nrows > 10000) {
        return NULL;
    }
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
    // const int LDA = nrows;
    LDA = nrows;
    
    board_init(inboard, nrows);
    memmove(outboard, inboard, nrows*ncols*sizeof(char));
    
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
        // pthread_barrier_wait(&barrier); 
        for (int i = 0; i < NUM_THREADS; ++i) {
	    pthread_join(worker_threads[i],NULL);
        }
	
        // can swap the board now
        SWAP_BOARDS(outboard, inboard);
    }
    
    return inboard;
}

void do_cell(char *outboard, char *inboard, int i, int j, const int size) {
    int inorth, isouth, jwest, jeast;
    char cell = BOARD(inboard, i, j);
    if (ALIVE(cell)) {

	if(TOKILL(cell)) {
	    KILL(BOARD(outboard, i, j));

	    jwest = mod(j-1, size);
	    jeast = mod(j+1, size);
	    inorth = mod(i-1, size);
	    isouth = mod(i+1, size);
	    
	    N_DEC(outboard, inorth, jwest);
	    N_DEC(outboard, inorth, j);
	    N_DEC(outboard, inorth, jeast);
	    N_DEC(outboard, i, jwest);
	    N_DEC(outboard, i, jeast);
	    N_DEC(outboard, isouth, jwest);
	    N_DEC(outboard, isouth, j);
	    N_DEC(outboard, isouth, jeast);
	    
	}
    }
    else {
	if(TOSPAWN(cell)) {
	    SPAWN(BOARD(outboard, i, j));

	    jwest = mod(j-1, size);
	    jeast = mod(j+1, size);
	    inorth = mod(i-1, size);
	    isouth = mod(i+1, size);

	    N_INC(outboard, inorth, jwest);
	    N_INC(outboard, inorth, j);
	    N_INC(outboard, inorth, jeast);
	    N_INC(outboard, i, jwest);
	    N_INC(outboard, i, jeast);
	    N_INC(outboard, isouth, jwest);
	    N_INC(outboard, isouth, j);
	    N_INC(outboard, isouth, jeast);
	}
    }
}

void board_init(char *board, int size) {
    int i, j;
    for (i = 0; i < size*size; i++) {
	if(board[i] == (char)1) {
	    board[i] = 0;
	    SPAWN(board[i]);
	}
    }

    int inorth, isouth, jwest, jeast;
    for(i = 0; i < size; i++) {
	for(j = 0; j < size; j++) {
	    if(ALIVE(BOARD(board, i, j))) {
		jwest = mod(j-1, size);
		jeast = mod(j+1, size);
		inorth = mod(i-1, size);
		isouth = mod(i+1, size);

		N_INC(board, inorth, jwest);
		N_INC(board, inorth, j);
		N_INC(board, inorth, jeast);
		N_INC(board, i, jwest);
		N_INC(board, i, jeast);
		N_INC(board, isouth, jwest);
		N_INC(board, isouth, j);
		N_INC(board, isouth, jeast);
	    }
	}
    }
}
