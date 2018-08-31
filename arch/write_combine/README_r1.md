
Processor model: Intel(R) Core(TM) i5-4210U CPU @ 1.70GHz

* According to results this processor has 9 WC buffers (for 20 distinct buffers split2 stops helping)
* The fact that results for strides 1-8 performance is comparable means that L2 cache prepatation time is relatively small and if buffering can help for 64-8 iterations it is sufficient.

![Intel Core i5](https://github.com/artpol84/poc/blob/master/arch/write_combine/img/plot_i5.png)

If 9 distinct buffers are used, no visible performance difference observed:
```
$ ./wc_bench 9 1
Cache subsystem:
Level: 1	Size: 32768
Level: 2	Size: 262144
Level: 3	Size: 3145728
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
	seq:	stride=1	13 cycles/B
	split2:	stride=1	8 cycles/B
	split2:	stride=2	12 cycles/B
	split2:	stride=4	15 cycles/B
	split2:	stride=8	13 cycles/B
	split2:	stride=16	13 cycles/B
	split2:	stride=32	11 cycles/B
	split2:	stride=64	12 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
	seq:	stride=1	14 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	13 cycles/B
	split2:	stride=4	12 cycles/B
	split2:	stride=8	13 cycles/B
	split2:	stride=16	14 cycles/B
	split2:	stride=32	23 cycles/B
	split2:	stride=64	37 cycles/B
Fit data to the level 3 of memory hirarchy (3145728B)
	seq:	stride=1	15 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	12 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	13 cycles/B
	split2:	stride=16	16 cycles/B
	split2:	stride=32	25 cycles/B
	split2:	stride=64	42 cycles/B
Fit data to the level 4 of memory hirarchy (12582912B)
	seq:	stride=1	15 cycles/B
	split2:	stride=1	13 cycles/B
	split2:	stride=2	13 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	16 cycles/B
	split2:	stride=16	17 cycles/B
	split2:	stride=32	31 cycles/B
	split2:	stride=64	58 cycles/B
```

If 10 buffers are used, advantage of array split become visible (~2x performance hit). Note that L1 cache is not affected:
```
$ ./wc_bench 10 1
Cache subsystem:
Level: 1	Size: 32768
Level: 2	Size: 262144
Level: 3	Size: 3145728
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
	seq:	stride=1	13 cycles/B
	split2:	stride=1	10 cycles/B
	split2:	stride=2	13 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	10 cycles/B
	split2:	stride=16	12 cycles/B
	split2:	stride=32	15 cycles/B
	split2:	stride=64	14 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
	seq:	stride=1	23 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	12 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	13 cycles/B
	split2:	stride=16	14 cycles/B
	split2:	stride=32	23 cycles/B
	split2:	stride=64	38 cycles/B
Fit data to the level 3 of memory hirarchy (3145728B)
	seq:	stride=1	21 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	15 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	13 cycles/B
	split2:	stride=16	17 cycles/B
	split2:	stride=32	25 cycles/B
	split2:	stride=64	44 cycles/B
Fit data to the level 4 of memory hirarchy (12582912B)
	seq:	stride=1	22 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	12 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	15 cycles/B
	split2:	stride=16	19 cycles/B
	split2:	stride=32	31 cycles/B
	split2:	stride=64	59 cycles/B
```

18 Buffers demonstrate maximum improvement (~3x):
```
$ ./wc_bench 18 1
Cache subsystem:
Level: 1	Size: 32768
Level: 2	Size: 262144
Level: 3	Size: 3145728
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
	seq:	stride=1	12 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	13 cycles/B
	split2:	stride=4	12 cycles/B
	split2:	stride=8	12 cycles/B
	split2:	stride=16	12 cycles/B
	split2:	stride=32	10 cycles/B
	split2:	stride=64	10 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
	seq:	stride=1	43 cycles/B
	split2:	stride=1	15 cycles/B
	split2:	stride=2	17 cycles/B
	split2:	stride=4	18 cycles/B
	split2:	stride=8	18 cycles/B
	split2:	stride=16	20 cycles/B
	split2:	stride=32	27 cycles/B
	split2:	stride=64	41 cycles/B
Fit data to the level 3 of memory hirarchy (3145728B)
	seq:	stride=1	45 cycles/B
	split2:	stride=1	14 cycles/B
	split2:	stride=2	12 cycles/B
	split2:	stride=4	15 cycles/B
	split2:	stride=8	17 cycles/B
	split2:	stride=16	20 cycles/B
	split2:	stride=32	29 cycles/B
	split2:	stride=64	46 cycles/B
Fit data to the level 4 of memory hirarchy (12582912B)
	seq:	stride=1	43 cycles/B
	split2:	stride=1	14 cycles/B
	split2:	stride=2	13 cycles/B
	split2:	stride=4	13 cycles/B
	split2:	stride=8	18 cycles/B
	split2:	stride=16	24 cycles/B
	split2:	stride=32	37 cycles/B
	split2:	stride=64	62 cycles/B
```

With 20 buffers advantage starts decreasing:
```
$ ./wc_bench 20 1
Cache subsystem:
Level: 1	Size: 32768
Level: 2	Size: 262144
Level: 3	Size: 3145728
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
	seq:	stride=1	12 cycles/B
	split2:	stride=1	12 cycles/B
	split2:	stride=2	10 cycles/B
	split2:	stride=4	10 cycles/B
	split2:	stride=8	12 cycles/B
	split2:	stride=16	9 cycles/B
	split2:	stride=32	9 cycles/B
	split2:	stride=64	13 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
	seq:	stride=1	43 cycles/B
	split2:	stride=1	22 cycles/B
	split2:	stride=2	20 cycles/B
	split2:	stride=4	21 cycles/B
	split2:	stride=8	23 cycles/B
	split2:	stride=16	26 cycles/B
	split2:	stride=32	32 cycles/B
	split2:	stride=64	38 cycles/B
Fit data to the level 3 of memory hirarchy (3145728B)
	seq:	stride=1	43 cycles/B
	split2:	stride=1	20 cycles/B
	split2:	stride=2	21 cycles/B
	split2:	stride=4	22 cycles/B
	split2:	stride=8	23 cycles/B
	split2:	stride=16	27 cycles/B
	split2:	stride=32	35 cycles/B
	split2:	stride=64	47 cycles/B
Fit data to the level 4 of memory hirarchy (12582912B)
	seq:	stride=1	43 cycles/B
	split2:	stride=1	20 cycles/B
	split2:	stride=2	24 cycles/B
	split2:	stride=4	24 cycles/B
	split2:	stride=8	25 cycles/B
	split2:	stride=16	31 cycles/B
	split2:	stride=32	41 cycles/B
	split2:	stride=64	69 cycles/B
```

And degrades to 0 for 32 buffers:
```
$ ./wc_bench 32 1
Cache subsystem:
Level: 1	Size: 32768
Level: 2	Size: 262144
Level: 3	Size: 3145728
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
	seq:	stride=1	10 cycles/B
	split2:	stride=1	13 cycles/B
	split2:	stride=2	12 cycles/B
	split2:	stride=4	12 cycles/B
	split2:	stride=8	13 cycles/B
	split2:	stride=16	10 cycles/B
	split2:	stride=32	13 cycles/B
	split2:	stride=64	15 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
	seq:	stride=1	42 cycles/B
	split2:	stride=1	43 cycles/B
	split2:	stride=2	43 cycles/B
	split2:	stride=4	42 cycles/B
	split2:	stride=8	44 cycles/B
	split2:	stride=16	42 cycles/B
	split2:	stride=32	41 cycles/B
	split2:	stride=64	41 cycles/B
Fit data to the level 3 of memory hirarchy (3145728B)
	seq:	stride=1	43 cycles/B
	split2:	stride=1	43 cycles/B
	split2:	stride=2	44 cycles/B
	split2:	stride=4	43 cycles/B
	split2:	stride=8	43 cycles/B
	split2:	stride=16	45 cycles/B
	split2:	stride=32	49 cycles/B
	split2:	stride=64	51 cycles/B
Fit data to the level 4 of memory hirarchy (12582912B)
	seq:	stride=1	47 cycles/B
	split2:	stride=1	44 cycles/B
	split2:	stride=2	44 cycles/B
	split2:	stride=4	48 cycles/B
	split2:	stride=8	44 cycles/B
	split2:	stride=16	48 cycles/B
	split2:	stride=32	54 cycles/B
	split2:	stride=64	69 cycles/B
```
