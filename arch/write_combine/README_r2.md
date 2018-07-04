
Processor model: Intel(R) Xeon(R) CPU E5-2680 v4 @ 2.40GHz

* According to results this processor has 8 WC buffers (for 18 distinct buffers split2 stops helping)

```
$ ./wc_bench 16
Cache subsystem:
Level: 1        Size: 32768
Level: 2        Size: 262144
Level: 3        Size: 36700160
cache line size: 64
Fit data to the level 1 of memory hirarchy (32768B)
        seq:    stride=1        4 cycles/B
        split2: stride=1        4 cycles/B
        split2: stride=2        4 cycles/B
        split2: stride=4        4 cycles/B
        split2: stride=8        4 cycles/B
        split2: stride=16       4 cycles/B
        split2: stride=32       4 cycles/B
        split2: stride=64       4 cycles/B
Fit data to the level 2 of memory hirarchy (262144B)
        seq:    stride=1        28 cycles/B
        split2: stride=1        5 cycles/B
        split2: stride=2        5 cycles/B
        split2: stride=4        6 cycles/B
        split2: stride=8        7 cycles/B
        split2: stride=16       9 cycles/B
        split2: stride=32       14 cycles/B
        split2: stride=64       26 cycles/B
Fit data to the level 3 of memory hirarchy (36700160B)
        seq:    stride=1        28 cycles/B
        split2: stride=1        5 cycles/B
        split2: stride=2        5 cycles/B
        split2: stride=4        6 cycles/B
        split2: stride=8        7 cycles/B
        split2: stride=16       9 cycles/B
        split2: stride=32       15 cycles/B
        split2: stride=64       27 cycles/B
Fit data to the level 4 of memory hirarchy (146800640B)
        seq:    stride=1        28 cycles/B
        split2: stride=1        5 cycles/B
        split2: stride=2        6 cycles/B
        split2: stride=4        6 cycles/B
        split2: stride=8        8 cycles/B
        split2: stride=16       11 cycles/B
        split2: stride=32       18 cycles/B
        split2: stride=64       35 cycles/B
```