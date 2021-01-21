# Solution

The `csim.c` file contains my cache simulation implementation.
The `trans.c` file contains my cache-optimized transpose
implementations.

## Part A

The implementation in `csim.c` is a straightforward translation of the
typical description of a set-associative LRU cache.

## Part B

The transpose routines in `trans.c` are optimized for fewest cache
misses on a few specific input cases.
`transpose_32` is optimized for `32 x 32` matrices,
`transpose_64` is optimized for `64 x 64` matrices,
and `transpose_generic` is optimized for `61 x 67` matrices (and
other matrix sizes not considered, sort of).
All assume a `(s=5, E=1, b=5)` cache, that is,
a `1KB` cache with `32 byte` block size.

The implementation ideas are documented with the functions,
and the implementations themselves are relatively straight forward.

## Testing

Copy the solutions into the `handout` directory and use the `driver.py`
script to run the tests used for grading.
```
cd ../handout
cp ../solution/*.c .
./driver.py
```
My cache simulator is fully correct for all the parameters tested.
The cache performance of my transpose functions is shown below in terms
of misses. For comparison, the performance of typical row-wise transpose
(`trans` in `trans.c`) and the target number of misses from the lab
description are also included.

| `N x M` | my transpose | baseline transpose | target |
| --- | --- | --- | --- |
| `32 x 32` | `275` | `1183` | `<300` |
| `64 x 64` | `1043` | `4723` | `<1300` |
| `61 x 67` | `1625` | `4423` | `<2000` |
