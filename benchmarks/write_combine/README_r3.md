
Processor model: Intel(R) Xeon(R) CPU E5-2680 0 @ 2.70GHz

* According to results this processor has 8 WC buffers (for 18 distinct buffers split2 stops helping)

```
$ ./wc_bench 18
Cache subsystem:
Level: 1        Size: 32768
Level: 2        Size: 262144
Level: 3        Size: 20971520
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
        seq:    stride=1        6 cycles/B
        split2: stride=1        6 cycles/B
        split2: stride=2        6 cycles/B
        split2: stride=4        6 cycles/B
        split2: stride=8        6 cycles/B
        split2: stride=16       6 cycles/B
        split2: stride=32       6 cycles/B
        split2: stride=64       6 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
        seq:    stride=1        20 cycles/B
        split2: stride=1        6 cycles/B
        split2: stride=2        7 cycles/B
        split2: stride=4        7 cycles/B
        split2: stride=8        7 cycles/B
        split2: stride=16       9 cycles/B
        split2: stride=32       11 cycles/B
        split2: stride=64       19 cycles/B
Fit data to the level 3 of memory hirarchy (20971520B)
        seq:    stride=1        20 cycles/B
        split2: stride=1        7 cycles/B
        split2: stride=2        7 cycles/B
        split2: stride=4        7 cycles/B
        split2: stride=8        8 cycles/B
        split2: stride=16       9 cycles/B
        split2: stride=32       13 cycles/B
        split2: stride=64       22 cycles/B
Fit data to the level 4 of memory hirarchy (83886080B)
        seq:    stride=1        20 cycles/B
        split2: stride=1        6 cycles/B
        split2: stride=2        7 cycles/B
        split2: stride=4        7 cycles/B
        split2: stride=8        9 cycles/B
        split2: stride=16       11 cycles/B
        split2: stride=32       17 cycles/B
		...
```
