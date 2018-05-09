#!/bin/sh

cd /

ISO_PATH=/home/chad/workspace/ubuntu-14.04.4-server-amd64.iso
if [ ! -f $ISO_PATH ]; then
	echo "iso is not exist"
        exit;
fi

if [ ! -f /mnt/newiso ]; then
        mkdir -p /mnt/newiso
fi

mount -o loop $ISO_PATH /mnt/newiso/

if [ -d /home/newiso ]; then
	rm -fr /home/newiso
fi

cp -fr /mnt/newiso /home/

sleep 1

cd /home

umount /mnt/newiso
if [ -d squashfs-root ]; then
	rm -fr squashfs-root
fi

unsquashfs newiso/install/filesystem.squashfs

cd squashfs-root/etc/

cp resolv.conf resolv.bak
cp apt/sources.list apt/sources.bak

cd -

cp /etc/resolv.conf squashfs-root/etc/
cp /etc/apt/sources.list squashfs-root/etc/apt/

cp /home/chad/workspace/mkiso/install_depends.sh squashfs-root/home/
