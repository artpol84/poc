This is a CPU "Write Combine Buffers (WCB) + SFENCE" analysis code that tries to estimate
the effect of sfence amount of WCB's using the following approach:

1. A variative number of NT-stores are performed sequentially
1. They are followed by a set of NO-OPs

The base idea of the test is the following:
* If WCBs are available then store operation takes few cycles and issue queue can be drained further allowing NO-OPs to be executed in parallel with actual I/O operations.
* If there is not enough WCBs, NO-OP operations will have to wait for some I/O operations to complete before they can be fetched from the queue. This delay will be observable. 