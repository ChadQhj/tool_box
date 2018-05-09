#!/bin/sh
if [ ! -f ./depends ]; then
        exit 0
fi

all_depends="$(cat ./depends)"

echo "all depends is :\n$all_depends"

echo y | apt-get install libtool > /dev/null 2>&1
for depend in $all_depends
do
	echo "install $depend,please waiting..."
        echo y | apt-get install $depend > /dev/null 2>&1
done


for depend in $all_depends
do
        installed="$(dpkg -l | grep $depend)"
        if [ -z "$installed" ]; then
                echo "failed to install system depend $depend"
        else
                echo "succeed to installed system depend $depend"
                sed -i /$depend/d ./depends
        fi
done

if [ ! -s ./depends ]; then
        echo "succeed to install all of the depends"
        rm -f ./depends
        exit 0
else
        echo "failed to install all of the depends"
        exit 1
fi
