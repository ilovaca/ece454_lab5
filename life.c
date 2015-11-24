/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <pthread.h>
#define NUM_THREADS 8
/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/
struct thread_argument_t
{
			char* outboard, 
			 char* inboard,
			  int nrows,
			  int ncols,
			  int gens_max,
			 int ith_slice,
};

worker_fuction_by_rows(struct thread_argument_t* arg)
{
	
}
/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/
char*
game_of_life (char* outboard, 
	      char* inboard,
	      const int nrows,
	      const int ncols,
	      const int gens_max)
{
	if (nrows < 32 && ncols < 32)
	  	return sequential_game_of_life (outboard, inboard, nrows, ncols, gens_max);
	else 
	{// bigger sized board, we use 8 threads 
		pthread_t worker_threads[NUM_THREADS];
		return parallel_game_of_life(outboard, inboard, nrows, ncols, gens_max);
	}
}


char*
parallel_game_of_life (char* outboard, 
			 char* inboard,
			 const int nrows,
			 const int ncols,
			 const int gens_max
			 pthread_t * worker_threads){

	const int LDA = *****;
	for (curgen = 0; curgen < gens_max; curgen++) {

		// start parallel code here
		/*Thought 1: slice the board by rows, i.e. horizontally*/
		//fire off the threads
	struct thread_argument_t* args = malloc(sizeof(struct thread_argument_t));
		args->outboard = outboard; 
		args->inboard = inboard;
			args-> nrows =  nrows;
			args-> ncols = ncols;
			args->gens_max =  gens_max;
			args->ith_slice = ith_slice;
		
		for (int i = 0; i < NUM_THREADS; ++i){
			// initialize the thread argument
			pthread_create(&(worker_threads[i],NULL, worker_fuction_by_rows,));
			worker_fuction_by_rows();
		}

	}
}


char*
sequential_game_of_life_optimized (char* outboard, 
			 char* inboard,
			 const int nrows,
			 const int ncols,
			 const int gens_max){
	/* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */
    const int LDA = nrows;
    int curgen, i, j;

    for (curgen = 0; curgen < gens_max; curgen++)
    {
        /* HINT: you'll be parallelizing these loop(s) by doing a
           geometric decomposition of the output */
        for (i = 0; i < nrows; i++)
        {
            for (j = 0; j < ncols; j++)
            {
                const int inorth = mod (i-1, nrows);
                const int isouth = mod (i+1, nrows);
                const int jwest = mod (j-1, ncols);
                const int jeast = mod (j+1, ncols);

                const char neighbor_count = 
                    BOARD (inboard, inorth, jwest) + 
                    BOARD (inboard, inorth, j) + 
                    BOARD (inboard, inorth, jeast) + 
                    BOARD (inboard, i, jwest) +
                    BOARD (inboard, i, jeast) + 
                    BOARD (inboard, isouth, jwest) +
                    BOARD (inboard, isouth, j) + 
                    BOARD (inboard, isouth, jeast);

                BOARD(outboard, i, j) = alivep (neighbor_count, BOARD (inboard, i, j));

            }
        }
        SWAP_BOARDS( outboard, inboard );
}
