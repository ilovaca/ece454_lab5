#ifndef _life_h
#define _life_h
#include <pthread.h>
/**
 * Given the initial board state in inboard and the board dimensions
 * nrows by ncols, evolve the board state gens_max times by alternating
 * ("ping-ponging") between outboard and inboard.  Return a pointer to 
 * the final board; that pointer will be equal either to outboard or to
 * inboard (but you don't know which).
 */

 int LDA;
#define BOARD(__board, __i, __j)  (__board[(__i) + LDA*(__j)])
#define NUM_THREADS 8

#define SWAP_BOARDS(b1, b2)  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define ALIVE(c) ((c >> 4) & 1)
#define SPAWN(c) (c |= (1 << (4)))
#define KILL(c) (c &= ~(1 << (4)))
#define TOKILL(c) ((c < (char)0x12 || cell > (char)0x13))
#define TOSPAWN(c) (c == (char)0x3)
#define N_INC(__board, __i, __j) (BOARD(__board, __i, __j) += 1)
#define N_DEC(__board, __i, __j) (BOARD(__board, __i, __j) -= 1)

char *
        game_of_life(char *outboard,
                     char *inboard,
                     const int nrows,
                     const int ncols,
                     const int gens_max);

/**
 * Same output as game_of_life() above, except this is not
 * parallelized.  Useful for checking output.
 */
char *
        sequential_game_of_life(char *outboard,
                                char *inboard,
                                const int nrows,
                                const int ncols,
                                const int gens_max);


char *
        parallel_game_of_life(char *outboard,
                              char *inboard,
                              const int nrows,
                              const int ncols,
                              const int gens_max
                              );


#endif /* _life_h */
