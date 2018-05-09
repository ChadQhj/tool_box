#!/bin/sh

if [ ! -f ids.tar.gz ]; then
	echo "file ids.tar.gz not found"
	exit
fi

tar -zxvf ids.tar.gz -C ./

if [ ! -d rootfs ]; then
	echo "tar failed"
	exit
fi

if [ -d /ids ]; then
	rm -fr /ids
fi

if [ -f /etc/ld.so.conf.d/ids-ld-so.conf ]; then
	rm -f /etc/ld.so.conf.d/ids-ld-so.conf
fi

mv ./rootfs/etc/ids-ld-so.conf /etc/ld.so.conf.d/
ldconfig

mkdir /ids
cp -r ./rootfs/ /ids
rm -fr ./rootfs
