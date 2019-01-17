This is a CPU "Write Combine Buffers (WCB) + SFENCE" analysis code that tries to estimate
the effect of sfence amount of WCB's using the following approach:

1. A variative number of NT-stores are performed sequentially
1. An SFENCE instruction is inserted in front of the last NT op to create a store fence between them.
1. They are followed by a set of NO-OPs

The base idea of the test is the following:
* Derived from WCB test
* The key difference is that here NO-OPs can not start even when WCB's are available because the last operation using WCB has to wait untill all previous WCB's are drained.
