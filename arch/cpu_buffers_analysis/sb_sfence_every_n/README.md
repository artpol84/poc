This is a CPU store buffer analysis code that tries to estimate the overhead of "sfence" instruction.

1. A variative number of stores are performed sequentially
1. Every Nth store an SFENCE instruction is inserted
1. The sequence of stores is followed by a set of NO-OPs
1. At the end a non-timed set of noops is inserted to ensure that the current iteration is completed.

The base idea of the test is the following:
* Derived from `sb` test
* Allows to insert "sfence" instructions every N store operations to analyze its overhead