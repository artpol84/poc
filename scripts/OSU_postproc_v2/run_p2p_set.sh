#!/bin/bash

mpirun_args=""

pml_lib=""
pml_iface=""
pml_tls=""

hcoll_lib=""
hcoll_iface=""

mxm_iface=""
mxm_tls=""
ucx_iface=""
ucx_tls=""

ucx_rndv_thr="auto"
ucx_zcopy_thr="auto"
hcoll_np=""

iter=""

large="no"

function set_ucx_env()
{
    iface=$1
    tls=$2
    rndv_thr=$3
    zcopy_thr=$4

    echo "-x UCX_NET_DEVICES=$iface -x UCX_TLS=$tls -x UCX_RNDV_THRESH=$rndv_thr -x UCX_ZCOPY_THRESH=$ucx_zcopy_thr"
}

function set_mxm_env()
{
    iface=$1
    tls=$2
    echo "-x MXM_RDMA_PORTS=$iface -x MXM_IB_PORTS=$iface -x MXM_TLS=$tls"
}

source env.sh

function run_binary()
{
    binary=$1
    if [ "$binary" = "osu_latency_mt" ]; then
        continue
    fi
    if [ -d "$binary" ]; then
        continue
    fi

    if [ -n "$UCX_BASE" ]; then
        UCS=${UCX_LIB_BASE}/libucs.so
        UCT=${UCX_LIB_BASE}/libuct.so
        UCP=${UCX_LIB_BASE}/libucp.so
        UCM=${UCX_LIB_BASE}/libucm.so
        export LD_PRELOAD=${UCS}:${UCT}:${UCP}:${UCM}
        preload="-x LD_PRELOAD"
    else
        preload=""
    fi

    # Calculate number of iterations
    is_bw=`echo "$binary" | grep "bw"`
    if [ -n "$is_bw" ]; then
        warmup=10
        reps=100
    else 
        # latency test
        warmup=100
        reps=1000
    fi

    large_suff=""
    if [ "$large" = "yes" ]; then
        # increase by 10
        warmup=0
        reps=$(( $reps * 300 ))
        large_suff="_large"
    fi

    fname="${i}_${pml_iface}_pml:${pml_lib}:${pml_tls}_hcoll:${hcoll_lib-none}:${hcoll_tls-none}${iter}${large_suff}.log"
    rm -f $fname
    echo "#$mpirun_cmd $preload $test_dir/$binary -i $reps -x $warmup" | tee $fname
    echo "#hosts: $SLURM_JOB_NODELIST" | tee -a $fname
    echo "LD_PRELOAD=$LD_PRELOAD"
    echo "#Test what hosts are involved" >> $fname
    tmp=`$mpirun_cmd $preload hostname`
    tmp2=""
    for h in $tmp; do
        tmp2="$tmp2 $h"
    done
    echo "#$tmp2" >> $fname
    echo "#Run the benchmark" >> $fname
    $mpirun_cmd $preload $binary -i $reps -x $warmup | tee -a $fname

}


mpirun_cmd="mpirun -np 2 --map-by node --bind-to core -mca btl self "

ulimit -c unlimited

test_dir=""
test_bin=""

while [ -n "$1" ]; do
    val=${1##*=}
    case "$1" in
    "-pml="*)
        pml_lib=${val%%/*}
        val=${val#*/}
        pml_iface=${val%%/*}
        val=${val#*/}
        pml_tls=$val
        ;;
    "-hcoll="*)
        hcoll_lib=${val%%/*}
        val=${val#*/}
        hcoll_tls=$val
        ;;
    "-iter="*)
        iter="_$val"
        ;;
    "-ucx-rndv-thr="*)
        ucx_rndv_thr=$val
        ;;
    "-ucx-zcopy-thr="*)
        ucx_zcopy-thr=$val
        ;;
    "-hcoll-np="*)
        hcoll_np=$val
        ;;
    "-dir="*)
        test_dir=$val
        ;;
    "-bin="*)
        test_bin=$val
        ;;
    "-large")
        large="yes"
        ;;
    *)
        echo "Unknown option: $1"
        exit 1
    esac
    shift
done

if [ -z "$pml_lib" ] || [ -z "$pml_iface" ] || \
    [ -z "$pml_tls" ]; then
    echo "ERROR: you need to provide PML settings"
    exit 1
fi


case "$pml_lib" in
    "mxm")
        mxm_iface=$pml_iface
        mxm_tls=$pml_tls
        tmp=`set_mxm_env $mxm_iface $mxm_tls`
        mpirun_cmd=$mpirun_cmd" $tmp"
        mpirun_cmd=$mpirun_cmd" -mca pml yalla"
        ;;
    "ucx")
        ucx_iface=$pml_iface
        ucx_tls=$pml_tls
        tmp=`set_ucx_env $ucx_iface $ucx_tls $ucx_rndv_thr $ucx_zcopy_thr`
        mpirun_cmd=$mpirun_cmd" $tmp"
        mpirun_cmd=$mpirun_cmd" -mca pml $pml_lib"
        ;;
    *)
        echo "Bad pml library: \"$pml_lib\", only support \"mxm\" and \"ucx\""
        exit 1
esac


if [ -z "$hcoll_lib" ]; then
    echo "WARNING: hcoll disabled"
    mpirun_cmd=$mpirun_cmd" -mca coll_hcoll_enabled 0 -mca coll ^hcoll "
else
    mpirun_cmd=$mpirun_cmd" -x HCOLL_SBGP=p2p"
    if [ -n "$hcoll_np" ]; then
        mpirun_cmd=$mpirun_cmd" -mca coll_hcoll_np $hcoll_np"
    fi
    case "$hcoll_lib" in
        "mxm")
            if [ -z "$mxm_iface" ]; then
                mxm_iface=$pml_iface
                mxm_tls=$hcoll_tls
                tmp=`set_mxm_env $mxm_iface $mxm_tls`
                mpirun_cmd=$mpirun_cmd" $tmp"
            else 
                if [ -n "$hcoll_tls" ]; then
                    echo "WARNING: Ignoring hcoll iface/tls settings, override with pml settings"
                fi
            fi
             mpirun_cmd=$mpirun_cmd" -x HCOLL_BCOL=mlnx_p2p"
            ;;
        "ucx")
            if [ -z "$ucx_iface" ]; then
                ucx_iface=$pml_iface
                ucx_tls=$hcoll_iface
                tmp=`set_ucx_env $ucx_iface $ucx_tls $ucx_rndv_thr $ucx_zcopy_thr`
                mpirun_cmd=$mpirun_cmd" $tmp"
            else 
                if [ -n "$hcoll_tls" ]; then
                    echo "WARNING: Ignoring hcoll iface/tls settings, override with pml settings"
                fi
            fi
            mpirun_cmd=$mpirun_cmd" -x HCOLL_BCOL=ucx_p2p"
            if [ -n "$UCX_PROFILE_MODE" ]; then
                mpirun_cmd=$mpirun_cmd" -x UCX_PROFILE_MODE"
            fi
            if [ -n "$UCX_PROFILE_FILE" ]; then
                mpirun_cmd=$mpirun_cmd" -x UCX_PROFILE_FILE"
            fi
            ;;
        *)
            echo "Bad hcoll library: \"$hcoll_lib\", only support \"mxm\" and \"ucx\""
            exit 1
    esac
fi

if [ -n "$test_dir" ]; then
    for i in `ls -1 $test_dir`; do
        run_binary $test_dir/$i
    done
else if [ -n "$test_bin" ]; then
    run_binary $test_bin
else
    echo "Nothing to run"
fi

