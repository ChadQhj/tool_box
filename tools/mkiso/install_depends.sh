#!/bin/sh
#need to chroot first

apt-get update

compile_depends="gcc g++ make flex bison automake libtool pkg-config"

for c_depend in $compile_depends
do
        echo "install compile depend:$c_depend,please wait..."
        sleep 2
        echo y | apt-get install $c_depend
done


snort_daq_depends="libpcap-dev libpcre3-dev libdnet-dev libdumbnet-dev zlib1g-dev libmysqlclient-dev"

for s_depend in $snort_daq_depends
do
        echo "install snort and daq depend:$s_depend,please wait..."
        sleep 2
        echo y | apt-get install $s_depend
done


other_application_depends="libssl-dev"

for o_depend in $other_application_depends
do
        echo "install application depend:$o_depend,please wait..."
        sleep 2
        echo y | apt-get install $o_depend
done


applications="apache2 php5 php5-mysql ssh mysql-server mysql-client zip"

for app in $applications
do
        echo "install applications:$app,please wait..."
        sleep 2
        echo y | apt-get install $app
done

web_depends="libmhash-dev wkhtmltopdf xvfb fonts-wqy-microhei ttf-wqy-microhei fonts-wqy-zenhei ttf-sqy-zenhei"

for w_depend in $web_depends
do
        echo "install web depends:$w_depend,please wait..."
        sleep 2
        echo y | apt-get install $w_depend
done


apt-get clean
apt-get autoremove

cd /etc/apache2
sed -i '1i\ServerName localhost' apache2.conf
make-ssl-cert /usr/share/ssl-cert/ssleay.cnf ./apache2.pem
a2enmod ssl
a2ensite default-ssl
a2dissite 000-default

