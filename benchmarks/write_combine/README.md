# Simple Write Combine (WC) benchmark
This is a POC demonstrating the effect of a Write Combine technology.

Links used to implement:
* [1] https://fgiesen.wordpress.com/2013/01/29/write-combining-is-not-your-friend/
* [2] https://mechanical-sympathy.blogspot.com/2011/07/write-combining.html
* [3] https://gist.github.com/1086581

Usage:
```shell
$ ./wc_bench <narrays> <niters>
```
* narrays - number of distinct arrays to be used (as different CPUs will likely to have different number of WC buffers)
* niters  - how many times to repeat the full access (in my tests 1 iter is enough)

In addititon to 2 patterns described in [2] I added a strided access to demonstrate how 64B WC buffer affects performance.

## [Result 1](https://github.com/artpol84/poc/blob/master/benchmarks/write_combine/README_r1.md) Intel(R) Core(TM) i5-4210U CPU @ 1.70GHz
