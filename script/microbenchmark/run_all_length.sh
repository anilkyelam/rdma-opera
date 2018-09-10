#!/usr/bin/env bash

if [[ $# -gt 2 ]]; then
    echo "$0 [<server ip>]"
fi

source ./config

#l=$((1*512*1024*1024))
#limit=$((1*512*1024*1024))

l=2
limit=2

count=1
verb="write"
mode="latency"
repeat=10
warmup=0
mr_count=1
tos=128

server="$1"
execpath=$RDMA_BENCH_EXECPATH

set -x
while [[ $l -le $limit ]]; do
    echo "Length = $l ..."
    $execpath -b $l -c $count -v $verb -m $mode -r $repeat -w $warmup --mr_count=$mr_count --tos=$tos $server
    (( l *= 2 ))
    echo ""
done

