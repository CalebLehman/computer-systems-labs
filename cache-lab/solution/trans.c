/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

/*
 * forward declaration function prototypes
 */
void transpose_64(int M, int N, int A[N][M], int B[M][N]);
void transpose_32(int M, int N, int A[N][M], int B[M][N]);
void transpose_generic(int M, int N, int A[N][M], int B[M][N]);
void copy_naive(int M, int N, int A[N][M], int A_i, int A_j, int B[M][N], int B_i, int B_j, int block);
void transpose_naive(int M, int N, int A[N][M], int A_i, int A_j, int B[M][N], int B_i, int B_j, int block);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if      ((M == 32) && (N == 32)) transpose_32(M, N, A, B);
    else if ((M == 64) && (N == 64)) transpose_64(M, N, A, B);
    else                             transpose_generic(M, N, A, B);
}

/*
 * Optimized for aligned A[64][64], B[64][64] matrices and a 1KB cache with a 32 byte block size.
 *
 * Based on a typical blocked transpose algorithm.
 * Alignment of matrices causes a lot of conflict misses, which are alleviated by using
 * unused blocks of B (which don't conflict with the memory addresses of the current blocks in A, B)
 * as temporary storage.
 */
void transpose_64(int M, int N, int A[N][M], int B[M][N]) {
    const int block = 8;
    for (int col = 0; col < M; col += block) {
        for (int row = 0; row < N; row += block) {
            // select temporary storage in B
            int c_i, c_j, d_i, d_j;
            c_i = d_i = col;
            c_j = row + 1*block;
            d_j = row + 2*block;
            if ((row+block)%N == col) {
                c_j += block;
                d_j += block;
            } else if ((row+2*block)%N == col) {
                d_j += block;
            }
            if (c_j >= N) {
                c_j -= N;
                c_i += block;
            }
            if (d_j >= N) {
                d_j -= N;
                d_i += block;
            }

            if (d_j >= M) {
                // no temporary storage available
                transpose_naive(M, N, B, col, row, A, row, col, block);
            } else {
                // copy from src block to temporary storage
                copy_naive(M, N, A, row,         col,         B, c_i, c_j, block/2);
                copy_naive(M, N, A, row,         col+block/2, B, c_i, c_j+block/2, block/2);
                copy_naive(M, N, A, row+block/2, col,         B, d_i, d_j, block/2);
                copy_naive(M, N, A, row+block/2, col+block/2, B, d_i, d_j+block/2, block/2);
                // transpose from temporary storage to dest block
                transpose_naive(M, N, B, c_i, c_j, B, col,         row,         block/2);
                transpose_naive(M, N, B, d_i, d_j, B, col,         row+block/2, block/2);
                transpose_naive(M, N, B, d_i, d_j+block/2, B, col+block/2, row+block/2, block/2);
                transpose_naive(M, N, B, c_i, c_j+block/2, B, col+block/2, row,         block/2);
            }
        }
    }
}

/*
 * Optimized for aligned A[32][32], B[32][32] matrices and a 1KB cache with a 32 byte block size.
 *
 * Based on a typical blocked transpose algorithm.
 * Alignment of matrices causes a lot of conflict misses, which are alleviated by using
 * by subdividing each block into 4 sub-blocks, and carefully moving managing the sub-blocks
 * to utilize lines already loaded into cache.
 */
void transpose_32(int M, int N, int A[N][M], int B[M][N]) {
    const int block = 8;
    for (int row = 0; row < N; row += block) {
        for (int col = 0; col < M; col += block) {
            // check for possible cache conflict between these blocks
            if (row == col) {
                /*
                 * transpose upper-left sub-block of A
                 * to lower-right sub-block of B (to avoid cache conflicts)
                 *
                 * transpose upper-right sub-block of A
                 * to lower-left sub-block of B, as normal
                 */
                for (int dr = 0; dr < block/2; ++dr) {
                    for (int dc = 0; dc < block/2; ++dc) {
                        int r = row+dr;
                        int c = col+dc;
                        B[c + block/2][r + block/2] = A[r][c];
                    }
                    for (int dc = block/2; dc < block; ++dc) {
                        int r = row+dr;
                        int c = col+dc;
                        B[c][r] = A[r][c];
                    }
                }
                /*
                 * transpose lower-left sub-block of A
                 * to upper-right sub-block of B, as normal
                 *
                 * transpose lower-right sub-block of A
                 * to upper-left sub-block of B (to avoid cache conflicts)
                 */
                for (int dr = block/2; dr < block; ++dr) {
                    for (int dc = 0; dc < block/2; ++dc) {
                        int r = row+dr;
                        int c = col+dc;
                        B[c][r] = A[r][c];
                    }
                    for (int dc = block/2; dc < block; ++dc) {
                        int r = row+dr;
                        int c = col+dc;
                        B[c - block/2][r - block/2] = A[r][c];
                    }
                }
                /*
                 * swap upper-left and lower-right sub-blocks of B to their final positions
                 */
                for (int dr = 0; dr < block/2; ++dr) {
                    for (int dc = 0; dc < block/2; ++dc) {
                        int r = row+dr;
                        int c = col+dc;
                        int tmp = B[r][c];
                        B[r][c] = B[r+block/2][c+block/2];
                        B[r+block/2][c+block/2] = tmp;
                    }
                }
            } else {
                for (int dr = 0; (dr < block) && (row+dr < N); ++dr) {
                    for (int dc = 0; (dc < block) && (col+dc < M); ++dc) {
                        int r = row+dr;
                        int c = col+dc;
                        B[c][r] = A[r][c];
                    }
                }
            }
        }
    }
}

/*
 * Generic transpose algorithm,
 * somewhat optimized for aligned A[61][67], B[67][61] matrices and a 1KB cache with a 32 byte block size.
 *
 * Based on a typical blocked transpose algorithm.
 * Uses a couple of temporary variables to save elements whose positions in A and B would
 * conflict in cache and move them last, to avoid reloading the current cache line from A,
 * if possible.
 */
void transpose_generic(int M, int N, int A[N][M], int B[M][N]) {
    const int block = 16;
    for (int j = 0; j < M; j += block) {
        for (int i = 0; i < N; i += block) {
            for (int di = 0; di < block; ++di) {
                // temporary variables to move elements that conflict in cache last,
                int temp1, temp2, dj1, dj2;
                dj1 = dj2 = -1;
                for (int dj = 0; dj < block; ++dj) {
                    if ((i+di >= N) || (j+dj >= M)) continue;
                    if ((((M*(i+di)+(j+dj))/8)&31) == (((N*(j+dj)+(i+di))/8)&31)) {
                        if (dj1 < 0) {
                            temp1 = A[i+di][j+dj];
                            dj1 = dj;
                        } else if (dj2 < 0) {
                            temp2 = A[i+di][j+dj];
                            dj2 = dj;
                        }
                    } else {
                        B[j+dj][i+di] = A[i+di][j+dj];
                    }
                }
                if (dj1 >= 0) B[j+dj1][i+di] = temp1;
                if (dj2 >= 0) B[j+dj2][i+di] = temp2;
            }
        }
    }
}

/*
 * Simple, block-based transpose algorithm.
 * Stores the transpose of A[A_i:A_i+block][A_j:A_j+block] in B[B_i:B_i+block][B_j:B_j+block]
 */
void transpose_naive(int M, int N, int A[N][M], int A_i, int A_j, int B[M][N], int B_i, int B_j, int block) {
    for (int i = 0; i < block; i += 2) {
        for (int j = 0; j < block; ++j) {
            B[B_i+j][B_j+i] = A[A_i+i][A_j+j];
        }
        for (int j = block-1; j >= 0; --j) {
            B[B_i+j][B_j+i+1] = A[A_i+i+1][A_j+j];
        }
    }
}

/*
 * Simple, block-based copy algorithm.
 * Copies A[A_i:A_i+block][A_j:A_j+block] to B[B_i:B_i+block][B_j:B_j+block]
 */
void copy_naive(int M, int N, int A[N][M], int A_i, int A_j, int B[M][N], int B_i, int B_j, int block) {
    for (int i = 0; i < block; i += 2) {
        for (int j = 0; j < block; ++j) {
            B[B_i+i][B_j+j] = A[A_i+i][A_j+j];
        }
        for (int j = block-1; j >= 0; --j) {
            B[B_i+i+1][B_j+j] = A[A_i+i+1][A_j+j];
        }
    }
}


/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
}
