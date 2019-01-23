This is a CPU store buffer analysis code that estimates the amount of store buffers.
The test is based on [1] and the implementation is based on [2]:

1. A variative number of stores are performed sequentially
1. They are followed by a set of NO-OPs

The base idea of the test is the following:
* If store buffers are available then store operation takes few cycles and CPU issue queue can be processed further allowing NO-OPs to be executed in parallel with actual I/O operations.
* If there is not enough store buffers, NO-OP operations will have to wait for all store operations to enter the pipeline before they can be fetched from the issue queue. This delay will be observable. 

Citations
[1] A. Morrison and Y. Afek. Fence-free work stealing on bounded tso processors. In Proceedings of the International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS), pages 413–426, 2014.
[2] https://nicknash.me/2018/04/07/speculating-about-store-buffer-capacity/