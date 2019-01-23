This test estimates the interoperation of CPU Write-Combine Buffers (WCB) and Store Buffers (SB)

The test algorithm:
1. A variative number of NT-stores are performed sequentially
1. They are followed by a fixed set of NO-OPs
1. Steps (1) and (2) are timed together, the expectation is to see that this time is equal to a plain NO-OP sequence execution
1. Another fixed set of NO-OPs are executed to make sure that all NT stores are complete before going to the next iteration. Otherwise an inter-iteration influence will skew the results.

The base idea of the test is the following:
* As every CPU instruction uses SB's NT-stores will also go to the SB first
* Despite the fact that we have only 10 WCBs in recent Intel CPUs we usually have much larger number of SBs.
* The expectation of the test is to see the results that are similar to 01_detect_SB_count but the gap between 42 and 43 NT-stores should be larger than for the regular stores.
