### Broadwell E5-2680 v4 @ 2.40GHz

```
$ taskset -c 1 ./cache_lat
Cache subsystem:
Level: 1        Size: 32768
Level: 2        Size: 262144
Level: 3        Size: 36700160
Cache Level 0: 2.000000 cycles, 0.833333 ns
Cache Level 1: 14.000000 cycles, 5.833333 ns
Cache Level 2: 37.838000 cycles, 15.765833 ns
```


### Broadwell E5-2620 v4 @ 2.10GHz (16 cores)
```
$ taskset -c 1 ./cache_lat
Cache subsystem:
Level: 1        Size: 32768
Level: 2        Size: 262144
Level: 3        Size: 20971520
Cache Level 0: 2.000000 cycles, 0.952381 ns
Cache Level 1: 15.000000 cycles, 7.142857 ns
Cache Level 2: 30.794000 cycles, 14.663810 ns
```

### Haswell E5-2697 v3 @ 2.60GHz
```
$ ./cache_lat
Cache subsystem:
Level: 1        Size: 32768
Level: 2        Size: 262144
Level: 3        Size: 36700160
Cache Level 0: 3.000000 cycles, 1.153846 ns
Cache Level 1: 11.000000 cycles, 4.230769 ns
Cache Level 2: 44.148000 cycles, 16.980000 ns
```
