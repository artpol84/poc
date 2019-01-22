This is a CPU store buffer analysis code that tries to estimate
the amount of store buffers using the following approach:

1. A variative number of stores are performed sequentially
1. They are followed by a set of NO-OPs

The base idea of the test is the following:
* If store buffers are available then store operation takes few cycles and issue queue can be drained further allowing NO-OPs to be executed in parallel with actual I/O operations.
* If there is not enough store buffers, NO-OP operations will have to wait for some I/O operations to complete before they can be fetched from the queue. This delay will be observable. 