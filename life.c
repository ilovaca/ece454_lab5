/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>


/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/
struct thread_argument_t {
    char *outboard;
    char *inboard;
    int nrows;
    int ncols;
    int ith_slice;
    int gens_max;
    pthread_barrier_t *barrier;
    pthread_mutex_t* boundary_locks;
};

void do_cell(char *outboard, char *inboard, int i, int j, const int LDA);
void board_init(char *board, int size);

// apply encoding to the board
void preprocessing_board(char* inboard, char* outboard, int nrows, int ncols) {
	for (int i = 0; i < nrows; ++i) {
		for (int j = 0; j < ncols; ++j) {
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

                        BOARD(outboard,i,j) = BOARD(inboard,i,j) << 4;


              			BOARD(outboard,i,j) += neighbor_count;
		}
	}
	memmove(inboard, outboard, nrows * ncols * sizeof(char));
}

inline void postprocessing_board(char* outboard, int nrows, int ncols) {
 	for (int i = 0; i < nrows*ncols; i++) {
	    outboard[i] = (char)ALIVE(outboard[i]);
	    // outboard[i+1] = ALIVE(outboard[i+1]);
  }
}


// Approach 2: we partition the board into blocks
void* worker_fuction_by_blocks(void *args) {

  struct thread_argument_t *arg = (struct thread_argument_t*)args;
  char *outboard = arg->outboard;
  char *inboard = arg->inboard;
  int nrows = arg->nrows;
  int ncols = arg->ncols;
  int ith_slice = arg->ith_slice / (int)sqrt(NUM_THREADS);
  int jth_slice = arg->ith_slice % (int)sqrt(NUM_THREADS);
  int slice_size = arg->nrows / sqrt(NUM_THREADS);

   for (int i = ith_slice * slice_size; i < (ith_slice + 1) * slice_size; ++i){

    for (int j = jth_slice * slice_size; j < (jth_slice + 1) * slice_size; ++j) {

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



void* worker_fuction_by_rows_encoding(void *args) {
	  struct thread_argument_t *arg = (struct thread_argument_t*)args;
	  char *outboard = arg->outboard;
	  char *inboard = arg->inboard;
	  int nrows = arg->nrows;
	  int ncols = arg->ncols;
	  int ith_slice = arg->ith_slice;
	  int gens_max = arg->gens_max;
	  pthread_barrier_t* barrier = arg->barrier;
	  pthread_mutex_t* boundary_locks = arg->boundary_locks;
	  int slice_size = arg->nrows / NUM_THREADS;
	  int start = ith_slice * slice_size;
	  int end = start + slice_size;

	for  (int curgen = 0; curgen < gens_max; ++curgen) {
	  	for (int i = start; i < end; ++i){
	  		// if (i > start + 1&& i < end - 1) 
	    	for (int j = 0; j < ncols; ++j) {
	    		// depending on the location of the cell, we choose to lock or not to lock
	    		if (i <= start + 1) {
	    			//lock upper boundary
	    			if (ith_slice != 0){

						pthread_mutex_lock(&boundary_locks[ith_slice]);
							do_cell(outboard, inboard, i, j, nrows);

		    			pthread_mutex_unlock(&boundary_locks[ith_slice]);
	    			} else {
	    				// the lock for the first slice is the same lock for the last slice
	    				pthread_mutex_lock(&boundary_locks[NUM_THREADS - 1]);
							do_cell(outboard, inboard, i, j, nrows);

		    			pthread_mutex_unlock(&boundary_locks[NUM_THREADS - 1]);
	    			}

	    		} 
	    		else if (i < end - 1) {
	    			// no need for lock
					do_cell(outboard, inboard, i, j, nrows);
	    		}
	    		else {
	    			//lock lower boundary
	    			if(ith_slice != NUM_THREADS - 1) {
		    			pthread_mutex_lock(&boundary_locks[ith_slice + 1]);
						do_cell(outboard, inboard, i, j, nrows);
		    			pthread_mutex_unlock(&boundary_locks[ith_slice + 1]);
	    			} else {
	    				pthread_mutex_lock(&boundary_locks[0]);
	    				do_cell(outboard, inboard, i, j, nrows);
		    			pthread_mutex_unlock(&boundary_locks[0]);
	    			}
	    		}
	    	}
	  }
		// int i,j;
		//  for (j = 0; j < ncols; j++) {
  //       	/*3 sections of the loop. We make the edge conditions between threads run separately in order
  //       	to avoid unnecessary code in the main body of the loop where the edge conditions aren't important */
  //       	for (i = start; i < start + 2; i++) {
  //       		// lock upper 
  //       		pthread_mutex_lock(&boundary_locks[ith_slice % 8]);
  //         		do_cell(outboard, inboard, i, j, nrows);
  //         		pthread_mutex_unlock(&boundary_locks[ith_slice % 8]);

  //       	}
  //         	for (i = start + 2; i < end - 2; i++) {
  //         		do_cell(outboard, inboard, i, j, nrows);
	 
  //           }
  //       	for (i = end - 2; i < end; i++) {
  //       		// lock lower
  //       		pthread_mutex_lock(&boundary_locks[(ith_slice + 1) % 8 ]);
  //  				do_cell(outboard, inboard, i, j, nrows);
  //  				pthread_mutex_unlock(&boundary_locks[(ith_slice + 1)% 8]);

  //       	}
  //       }


	  pthread_barrier_wait(barrier);
	  // each thread copies his portion of data
	  memcpy(inboard + start, outboard + start, slice_size * ncols * sizeof (char));
	  
	  pthread_barrier_wait(barrier);
	}
	  return NULL;
}

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
    if (nrows < 32) {
        return sequential_game_of_life(outboard, inboard, nrows, ncols, gens_max);
    }
    else if (nrows > 10000) {
        return NULL;
    }
    else {
        return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max);
    }
}



char * parallel_game_of_life(char *outboard,
                      char *inboard,
                      const int nrows,
                      const int ncols,
                      const int gens_max) {
	
	pthread_t worker_threads[NUM_THREADS];

    pthread_mutex_t boundary_locks[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
    	pthread_mutex_init(&boundary_locks[i], NULL);
    }
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, NUM_THREADS);
    // const int LDA = nrows;

  	LDA = nrows;
  	// preprocessing_board(inboard, outboard, nrows,  ncols);
  	board_init(inboard, nrows);
  	memmove(outboard, inboard, nrows * ncols * sizeof(char));
  	struct thread_argument_t args[NUM_THREADS];

  	for (int i = 0; i < NUM_THREADS; ++i) {
		    args[i].outboard = outboard;
		    args[i].inboard = inboard;
		    args[i].nrows = nrows;
		    args[i].ncols = ncols;  
		    args[i].ith_slice = i;
		    args[i].gens_max = gens_max;
		    args[i].barrier = &barrier;
		    args[i].boundary_locks = boundary_locks;
            pthread_create(&worker_threads[i], NULL, worker_fuction_by_rows_encoding, &args[i]);
        }

    for (int i = 0; i < NUM_THREADS; ++i) {
	    pthread_join(worker_threads[i],NULL);
    }

    postprocessing_board(inboard,nrows,ncols);

    return inboard;
}


void do_cell(char *outboard, char *inboard, int i, int j, const int size) {
    int inorth, isouth, jwest, jeast;
    char cell = BOARD(inboard, i, j);
    if (ALIVE(cell)) {
		if(TOKILL(cell)) {
			// if this cell is alive and it should die, then we need to
			// do two things: mark it alive....
			    KILL(BOARD(outboard, i, j));

			    // ... and decrement the counter in its neighbors
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
    	// this cell is dead
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
		if(board[i] == 1) {
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

