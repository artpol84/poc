This test evaluate the situation where one WCB store is followed by SFENCE followed by a set of regular stores.

1. One NT-store is performed
1. SFENCE instruction is called
1. A variative set of regular (cached) stores are performed

The base idea of the test is the following:
* Regular stores have low latency when they fit L1 cache
* However NT-store with higher latency followed by SFENCE will force regular stores to sit in the SB's until NT store is complete
* As the result up to ~42 regular stores there will be no visible overhead with a jump afterwards
