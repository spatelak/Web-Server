#!/bin/bash

testcase=${0%\.*}
serverout="$testcase.server.out"
telnetout="$testcase.telnet.out"
cmpfile="$testcase.cmp"

http="bin/http"
if [[ "$1" = "-e" ]]; then
    http="$2"
    shift
    shift
fi

verbose="$1"

$http -R > $serverout 2>&1 &
server_pid=$!

sleep 0.2

isalive=$(ps -u $USER | grep $server_pid | uniq | wc -l)

if [[ $isalive != "1" ]]; then
    echo "Could not start server" > $cmpfile
    exit 1
fi

port=$(get-port.sh $server_pid)

# start netcats
nc 127.0.0.1 $port &
nc1=$!

nc 127.0.0.1 $port &
nc2=$!

nc 127.0.0.1 $port &
nc3=$!

sleep 0.5

# ensure 4 processes are running
proccount=$(ps -Lfu $USER | grep $http | grep -vE 'bash|grep' | awk '{print $2}' | uniq | wc -l)
threadcount=$(ps -Lfu $USER | grep $http | grep -vE 'bash|grep' | awk '{print $4}' | uniq | wc -l)
ret=0

if [[ $proccount != "1" || $threadcount != "4" ]]; then
    echo "Expected 1 process and 4 threads; got $proccount processes and $threadcount threads" > $cmpfile
    ret=1
fi

for pid in $nc1 $nc2 $nc3 $server_pid; do
    kill -SIGKILL $pid
    wait $pid 2> /dev/null
    sleep 0.1
done

if [[ "$verbose" != "-v" ]]; then
    rm -f $serverout
    if [[ "$ret" = "0" ]]; then
        rm -f $cmpfile
    fi
fi

exit $ret
