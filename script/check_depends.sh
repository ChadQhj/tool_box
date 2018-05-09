#!/bin/sh

rm -f ./depends

all_depends="build-essential gcc g++ libmysqlclient-dev libdnet libdnet-dev libtool automake autoconf libpcre3 libpcre3-dev libpcap-dev \
libpcap-dev flex bison zlib1g-dev libdumbnet-dev"

for depend in $all_depends
do
	installed="$(dpkg -l | grep $depend)"
	if [ -z "$installed" ]; then
		echo "$depend" >> ./depends
	fi
done
