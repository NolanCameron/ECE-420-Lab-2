#!/bin/bash

size=$1
server_ip=$2
server_port=$3

clientRepeat=3
attackerRepeat=3
term_server=true
server=main
client=client
attacker=attacker

if [[ -z $3 ]];
then
    echo "need parameters"
fi

echo "Launching Server..."
setsid stdbuf -oL -eL ./${server} ${size} ${server_ip} ${server_port} > _server.log &
server_pgid=$!
sleep 1  # wait a bit for the server to launch before running the client

echo "Server pid: ${server_pgid}"

attempt=0
while [[ ${attempt} -ne ${attackerRepeat} ]]; do
    let attempt+=1
    echo "Running Attacker -- Round ${attempt}..."
    ./${attacker} ${size} ${server_ip} ${server_port} <<< "SpacelessString_${attempt}" >> _attacker_${attempt}.log
    sleep 0.5
done

attempt=0
while [[ ${attempt} -ne ${clientRepeat} ]]; do
    let attempt+=1
    echo "Running Client -- Round ${attempt}..."
    ./${client} ${size} ${server_ip} ${server_port} <<< "SpacelessString_${attempt}" >> _client_${attempt}.log
    sleep 0.5
done



if ${term_server}; then
    echo "Terminating server and related procs..."
    kill -- -${server_pgid}
else
    echo "Exiting without terminating the server..."
fi
