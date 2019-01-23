This is a CPU Write Combine Buffers (WCB) analysis code that estimates the amount of WCB's using the following approach:

1. A variative number of distinct Non-Temporal (NT) stores are performed sequentially in a tight loop.

The base idea of the test is the following:
* Write Combining can happen if same 64B are accessed multiple times.
* This test varies number of distinct locations that are being accessed.
* If the number of distinct NT stores is less or equal to the number of WCB's then the overhead of store operations will be low.
* As long as the number of distinct NT stores grows beyond the number of WCB's, the latter NT sores will have to wait for the previous NT ops to be completed.
* This change is expected to be observable.
