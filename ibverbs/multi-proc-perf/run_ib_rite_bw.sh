#!/bin/bash

TRANSP=$1
TYPE=$2
if [ "$TYPE" != "cli" ] && [ "$TYPE" != "srv" ]; then
    echo "Wrong role. Only 'srv' and 'cli' supported"
    exit 1
fi

shift 2
OPTIONS="$@"

run_experiment()
{
    mypid=$$
    nproc=$1
    export MLX5_SINGLE_THREADED=1 
    
    # Synchronize client/server using NFS
    if [ "$TYPE" = "cli" ]; then
        while [ ! -f "srv_ready.$nproc" ]; do
            rm -f "cli_ready.$nproc"
            sleep 0.1
            touch "cli_ready.$nproc"
        done
        rm -f "srv_ready.$nproc" 
        rm -f "cli_ready.$nproc"
    fi
    
    for i in `seq 0 $(( $nproc - 1))`; do
        taskset -c $i \
            ib_write_bw -d mlx5_0 -s 8 -p $((1200 + $i)) --output=message_rate -F --inline_size=8 -q 1 -l 1 -D 5 -c $TRANSP $OPTIONS > out.${mypid}.$i &
    done
    if [ "$TYPE" = "srv" ]; then
        # Ensure that all servers are ready
        for i in `seq 0 $(( $nproc - 1))`; do
            flag=1
            while [ "$flag" -eq 1 ]; do
                tmp=`netstat -antp 2>/dev/null | grep -e "0\.0\.0\.0:"$((1200 + $i))" " | grep -e "ib_write_bw"`
                if [ -n "$tmp" ]; then
                    flag=0
                fi
            done
        done
        # Allow connection
        touch "srv_ready.$nproc"
        echo "1" > "srv_ready.$nproc"
    fi

    wait

    for i in `seq 0 $(( $nproc - 1))`; do
        cat out.${mypid}.$i
    done | awk 'BEGIN{ sum = 0.0; } { sum += $1; } END{ print sum }'
    rm out.${mypid}.*
}

pkill -KILL ib_write_bw
rm -f "srv_ready.*"
rm -f out\.*\.*

for i in `seq 1 28`; do
#i=19
    mr=`run_experiment $i`
    echo -e "$i\t$mr"
done