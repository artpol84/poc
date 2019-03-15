#!/bin/bash

PERF_CMD=perf

evlist_cpu_base="
    cycles		\
    bus-cycles		\
    cache-misses	\
    cache-references	\
    branches		\
    branch-misses	\
    instructions	\
    ref-cycles		\
"

elvist_mem_base="	\
    mem-loads		\
    mem-stores		\
    node-load-misses	\
    node-loads		\
    node-store-misses	\
    node-stores		\
"
evlist_OS="		\
    context-switches	\
    cpu-migrations	\
    page-faults
"

evlist_branch="		\
    branch-load-misses	\
    branch-loads	\
"

evlist_L1="
    L1-dcache-loads 		\
    L1-dcache-load-misses 	\
    L1-dcache-stores		\
"
evlist_L2="
    l2_rqsts.references 		\
    l2_rqsts.miss l2_rqsts.all_rfo	\
    l2_rqsts.rfo_miss			\
"

evlist_L3="
    LLC-loads 					\
    longest_lat_cache.miss 			\
    longest_lat_cache.reference			\
    llc_misses.mmio_read			\
    llc_misses.mmio_write			\
    llc_misses.pcie_non_snoop_write		\
    llc_misses.pcie_read			\
    llc_misses.pcie_write			\
    llc_misses.rfo_llc_prefetch			\
    llc_misses.uncacheable			\
    llc_references.pcie_ns_partial_write	\
    llc_references.pcie_read			\
    llc_references.pcie_write			\
"

evlist_CC="	
    offcore_requests_buffer.sq_full					\
    offcore_requests.all_data_rd 					\
    offcore_requests_outstanding.all_data_rd 				\
    offcore_requests.demand_data_rd 					\
    offcore_response.all_requests.llc_hit.any_response 			\
    offcore_response.all_data_rd.llc_hit.hit_other_core_no_fwd 		\
    offcore_response.all_data_rd.llc_hit.hitm_other_core 		\
    offcore_response.all_reads.llc_hit.hit_other_core_no_fwd 		\
    offcore_response.all_reads.llc_hit.hitm_other_core 			\
    offcore_response.all_requests.llc_hit.any_response 			\
    offcore_response.demand_data_rd.llc_hit.hit_other_core_no_fwd 	\
    offcore_response.demand_data_rd.llc_hit.hitm_other_core 		\
    offcore_requests.demand_rfo 					\
    offcore_requests_outstanding.cycles_with_demand_rfo  		\
    offcore_response.all_rfo.llc_hit.hit_other_core_no_fwd 		\
    offcore_response.all_rfo.llc_hit.hitm_other_core 			\
    offcore_response.demand_rfo.llc_hit.hit_other_core_no_fwd 		\
    offcore_response.demand_rfo.llc_hit.hitm_other_core 		\
"

evlist_lock="				\
    mem_uops_retired.lock_loads 	\
    lock_cycles.cache_lock_duration	\
"

evlist_mem=" \
    machine_clears.memory_ordering				\
    mem_trans_retired.load_latency_gt_8               		\
    mem_trans_retired.load_latency_gt_4               		\
    mem_trans_retired.load_latency_gt_32              		\
    mem_trans_retired.load_latency_gt_16              		\
    mem_trans_retired.load_latency_gt_64              		\
    mem_trans_retired.load_latency_gt_128             		\
    mem_trans_retired.load_latency_gt_256             		\
    mem_trans_retired.load_latency_gt_512             		\
    offcore_response.all_data_rd.llc_miss.any_response		\
    offcore_response.all_data_rd.llc_miss.local_dram  		\
    offcore_response.all_data_rd.llc_miss.remote_dram 		\
    offcore_response.all_data_rd.llc_miss.remote_hit_forward	\
    offcore_response.all_data_rd.llc_miss.remote_hitm 		\
    offcore_response.demand_data_rd.llc_miss.any_response	\
    offcore_response.demand_data_rd.llc_miss.local_dram		\
    offcore_response.all_reads.llc_miss.any_response  		\
    offcore_response.all_reads.llc_miss.local_dram    		\
    offcore_response.all_reads.llc_miss.remote_dram   		\
    offcore_response.all_reads.llc_miss.remote_hit_forward	\
    offcore_response.all_reads.llc_miss.remote_hitm   		\
    offcore_response.all_rfo.llc_miss.any_response    		\
    offcore_response.all_rfo.llc_miss.local_dram      		\
"

evlist_res="				\
    resource_stalls.any			\
    resource_stalls.rob			\
    resource_stalls.rs                  \
    resource_stalls.sb			\
    uops_executed.stall_cycles		\
    uops_executed.core			\
    uops_issued.any			\
    uops_issued.core_stall_cycles	\
    uops_retired.all			\
    uops_retired.core_stall_cycles      \
    uops_retired.retire_slots           \
    uops_retired.stall_cycles		\
    uops_retired.total_cycles		\
"


function run_cmd()
{
    type=$1
    pid=$2
    event=$3
    cmd="$PERF_CMD stat -e $event "
    if [ "$type" = "thread" ]; then
	cmd="$cmd -t $pid "
    else
	cmd="$cmd -p $pid "
    fi
#    echo "$cmd"
    $cmd sleep 1 |& grep "$event"
}

function run_set()
{
    type=$1
    shift
    pid=$1
    shift
    for ev in $@; do
	run_cmd $type $pid $ev
    done
}

type="process"
if [ "$1" = "thread" ]; then
    type="thread"
    shift
fi
pid=$1

echo "Run in \"$type\" mode"
echo "Basic CPU events"
run_set $type $pid $evlist_cpu_base

echo "Basic CPU events (mem access)"
run_set $type $pid $elvist_mem_base

echo "OS events"
run_set $type $pid $evlist_OS

echo "Branch events"
run_set $type $pid $evlist_branch

echo "L1 stat"
run_set $type $pid $evlist_L1

echo "L2 stat"
run_set $type $pid $evlist_L2

echo "LLC stat"
run_set $type $pid $evlist_L3

echo "Cache-Coherency stat"
run_set $type $pid $evlist_CC

echo "Atomics stat"
run_set $type $pid $evlist_lock

echo "Memory"
run_set $type $pid $evlist_mem

echo "Resources"
run_set $type $pid $evlist_res
