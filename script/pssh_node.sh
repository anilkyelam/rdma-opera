#!/usr/bin/env bash

# Configuration
host=$(hostname -s)
hosts=$(cat hosts.config | paste -s -d "," -)
host_list=($(cat hosts.config))
host_count=${#host_list[@]}

# find the index
index=-1
for i in "${!host_list[@]}"; do
    if [[ "${host_list[$i]}" = "$host" ]]; then
        index=$i
        break
    fi
done

if [ $index -lt 0 ]; then
    echo >&2 "Host $host not included in config file, exiting ..."
    exit 2
fi

curr_index=$index
curr_host="${host_list[$curr_index]}"
next_index=$(echo "($curr_index + 1) % $host_count" | bc)
next_host="${host_list[$next_index]}"

# Launch
cd ../build
echo "launch with option --hosts=$hosts --host=$curr_host --index=$curr_index --next-index=$next_index --next-host=$next_host"

# udaddy test
: '
udaddy &
pid=$!
udaddy -s $next_host
wait $pid
#'

# rdma_server test
: '
rdma_server &
pid=$!
rdma_client -s $next_host
wait $pid
#'

# Mallenox benchmark tool

: '
client=$curr_host
server=$next_host
client_log="${client}_to_${server}.client.out"
server_log="${client}_to_${server}.server.out"
duration="5s"
ib_flags="-F -R -D $duration --report_gbits"

echo "client_log=$client_log, server_log=$server_log, duration=$duration, ib_flags=$ib_flags"
#'

# ib_read_bw test
: '
ib_read_bw $ib_flags > $server_log &
pid=$!
sleep 1
ib_read_bw $ib_flags $server | tee $client_log
wait $pid
#'

# ib_write_bw test
: '
ib_write_bw $ib_flags > $server_log &
pid=$!
sleep 1
ib_write_bw $ib_flags $server | tee $client_log
wait $pid
#'
