#! /bin/sh

host=$1
port=$2
delay=$3

if [ "$host" = "" ] || [ "$port" = "" ]; then
	echo "Usage : $0 host port"
	exit
fi

if [ "$delay" = "" ]; then
	delay=30;
fi

echo "Sending GPS datas to host -$host- at port -$port-"

# telnet hs been removed for nc (Netcat)
cat $(dirname $0)/data-ok.txt > | nc -i "$delay" $1 $2

