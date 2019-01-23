This is a CPU "Write Combine Buffers (WCB) + SFENCE" analysis code that tries to estimate
the effect of sfence amount of WCB's using the following approach:

1. A variative number of NT-stores are performed sequentially
1. An SFENCE instruction is inserted after the first NT op to create a store fence between them.
1. They are followed by a set of NO-OPs

The base idea of the test is the following:
* Derived from WCB test
