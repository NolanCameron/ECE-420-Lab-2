#!/bin/bash

size=$1
server_ip=$2
server_port=$3

repeat=3
term_server=true
server=main
client=client

if [[ -z $3 ]];
then
    echo "need parameters"
fi

echo "Launching Server..."
setsid stdbuf -oL -eL ./${server} ${size} ${server_ip} ${server_port} > _server.log &
server_pgid=$!
sleep 1  # wait a bit for the server to launch before running the client

attempt=0
while [[ ${attempt} -ne ${repeat} ]]; do
    let attempt+=1
    echo "Running Client -- Round ${attempt}..."
    ./${client} ${size} ${server_ip} ${server_port} #<<< "SpacelessString_${attempt}" >> _client.log
    sleep 0.5
done

if ${term_server}; then
    echo "Terminating server and related procs..."
    kill -- -${server_pgid}
else
    echo "Exiting without terminating the server..."
fi
