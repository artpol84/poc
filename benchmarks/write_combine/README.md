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

## Result 1:
```
model name      : Intel(R) Core(TM) i5-4210U CPU @ 1.70GHz
```
* According to results this processor has 9 WC buffers (for 20 distinct buffers split2 stops helping)
* The fact that results for strides 1-8 performance is comparable means that L2 cache prepatation time is relatively small and if buffering can help for 64-8 iterations it is sufficient.
```
$ ./wc_bench 16 1
cache line size: 64
seq:	12408306494
split2:	4027402662
split2:	stride=1 4011590618
split2:	stride=2 4296504128
split2:	stride=4 4557828418
split2:	stride=8 4992765724
split2:	stride=16 6483988180
split2:	stride=32 9758034676
split2:	stride=64 17551834752
```
```
$ ./wc_bench 18 1
cache line size: 64
seq:	13849068184
split2:	5510297980
split2:	stride=1 5461435028
split2:	stride=2 5817118970
split2:	stride=4 6234176780
split2:	stride=8 6931894490
split2:	stride=16 8774713130
split2:	stride=32 12430639390
split2:	stride=64 19781894214
```
```
$ ./wc_bench 20 1
cache line size: 64
seq:	16602473280
split2:	15257938822
split2:	stride=1 15047536594
split2:	stride=2 15057847276
split2:	stride=4 15525145424
split2:	stride=8 14752383108
split2:	stride=16 16417927768
split2:	stride=32 18570614440
split2:	stride=64 23458021980
```

## Results 2
```
model name      : Intel(R) Xeon(R) CPU E5-2680 v4 @ 2.40GHz
```
Same observations as for `Results 1`
```
$ ./wc_bench 16 1
cache line size: 64
seq:    7985897112
split2: 1810096046
split2: stride=1 1760218200
split2: stride=2 1971162304
split2: stride=4 2034196658
split2: stride=8 2448090910
split2: stride=16 3264881794
split2: stride=32 5209887676
split2: stride=64 9848617540
```

```
$ ./wc_bench 18 1
cache line size: 64
seq:    8975615694
split2: 4949352845
split2: stride=1 4957767719
split2: stride=2 5084605970
split2: stride=4 5261310254
split2: stride=8 5621440832
split2: stride=16 6405252732
split2: stride=32 7898591332
split2: stride=64 11021645000
```

```
$ ./wc_bench 20 1
cache line size: 64
seq:    9974892706
split2: 9813309520
split2: stride=1 9812278590
split2: stride=2 9864740446
split2: stride=4 9927591188
split2: stride=8 10105013874
split2: stride=16 10447209044
split2: stride=32 11072929868
split2: stride=64 12308608526
```