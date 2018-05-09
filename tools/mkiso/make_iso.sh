#!/bin/sh

cd /home/squashfs-root/etc
if [ -f resolv.bak ]; then
	rm -f resolv.conf
	mv resolv.bak resolv.conf
fi
if [ -f apt/sources.bak ]; then
	rm -f apt/sources.list
	mv apt/sources.bak sources.list
fi

cd /home
if [ -f squashfs-root/home/install_depends.sh ]; then
	rm -f squashfs-root/home/install_depends.sh
fi

#modify filesystem.manifest
chroot squashfs-root dpkg-query -W --showformat='${Package} ${Version}\n' > newiso/install/filesystem.manifest

#create filesystem.squashfs
rm newiso/install/filesystem.squashfs
mksquashfs squashfs-root newiso/install/filesystem.squashfs

#modify filesystem.size
du -sx --block-size=1 squashfs-root/ | cut -f1 > newiso/install/filesystem.size

#modify md5.txt
cd newiso
find -type f -print0 | xargs -0 md5sum | grep -v isolinux/boot.cat | tee md5sum.txt

#create iso 
if [ - ../surveyor-os.iso ]; then
	rm -f ../surveyor-os.iso
fi
version=$(sh /home/chad/workspace/mkiso/version)
if [ -z $version ]; then
	version=1.0.0.0
fi
mkisofs -D -r -V "suveyor-os" -cache-inodes -J -l -b isolinux/isolinux.bin -c isolinux/boot.cat -no-emul-boot -boot-load-size 4 -boot-info-table -o ../surveyor-os-V$version.iso .

