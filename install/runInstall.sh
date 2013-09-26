#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# with ". script.sh" the script will run in current context not as a new process
. installVars.sh

if [ "$?" != "0" ]; then
	echo "could not load settings from installVars.sh" 1>&2
	exit 1
fi
if [ -z "$installDir" ]; then
	echo "could not load settings from installVars.sh" 1>&2
	exit 1
fi

pushd . > /dev/null
cd ..
gitDaemonDir=`pwd`
popd > /dev/null

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
##for using "apt-get install" wlan is needed, you can do it by manually add a wlan config:
# sudo wpa_passphrase [wlan-ssid] [pw] >> /etc/wpa_supplicant/wpa_supplicant.conf

if [ "$1" != "nopackages" ]; then
	echo "install needed packages"
	dpkg -iG apt_archives/*deb
	#apt-get update
	#apt-get install php-apc php5-mysql php5-memcached memcached libmemcached10 php5-gd libgd2-xpm php5-common libapache2-mod-php5filter php5-cli isc-dhcp-server isc-dhcp-common isc-dhcp-client hostapd
fi


#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

if [ -e $webmain/old ]; then
	echo "/var/www/old already exist do not move files"
else
	echo "move files in var/www to /var/www/old"
	mkdir $webmain/old
	mv $webmain/* /$webmain/old
fi

echo -e "\nprepare web root files"
cp web_root/* $webmain/
chown $userToUse:$webuser $webmain/*

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo -e "\nprepare web hw files"
mkdir $webmain/hw
ln -s  /dev/shm/status  $webmain/hw/status
ln -s  /dev/shm/command  $webmain/hw/command

echo -e "\ncreate $webmain/hw/sendcommand.php"
cat << EOF > $webmain/hw/sendcommand.php
<?php
error_reporting(E_ERROR | E_WARNING | E_PARSE | E_NOTICE);

if (isset(\$_REQUEST['command'])) {
	\$command=$_REQUEST['command'];
} else {
	\$command="";
}

\$fw = fopen("command", "w");
if (fwrite(\$fw, "\$command")) {
	//echo "Writing #$command OK<br>";
}
fclose(\$fw);
?>
EOF
chown $userToUse:$webuser $webmain/hw/sendcommand.php
chmod 0644 $webmain/hw/sendcommand.php


#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo -e "\n\nrun install_sync.sh"
echo "####################################################"
./install_sync.sh
cp main_yii_config.php $installDir/sync/main_yii_config.php
chown $userToUse:$userToUse $installDir/sync/main_yii_config.php

echo -e "\n\ninit db"
echo "####################################################"
$installDir/sync/init_db.sh

echo -e "\ncreate symbolic links"
rm $installDir/middleware
ln -s $gitDaemonDir/middleware $installDir/middleware
rm $installDir/daemon
ln -s $gitDaemonDir/daemon $installDir/daemon
rm $webmain/manualmode
ln -s $gitDaemonDir/manualmode $webmain/manualmode
rm $gitDaemonDir/manualmode/status
ln -s  /dev/shm/status  $gitDaemonDir/manualmode/status
rm $gitDaemonDir/manualmode/data.txt
ln -s  /var/log/EveryCook_Daemon.log  $gitDaemonDir/manualmode/data.txt

ln -s /dev/shm/ecdaemon.log $installDir/daemon/ecdaemon.log
ln -s /dev/shm/ecmiddleware.log $installDir/middleware/ecmiddleware.log

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo -e "\n\nrun configure.sh"
echo "####################################################"
./configure.sh

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo -e "\n\nrun createInstallSettingsScript.sh"
echo "####################################################"
./createInstallSettingsScript.sh

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

if [ "$1" != "norestart" ] && [ "$2" != "norestart" ]; then
	echo -e "\nrestart wlan(as accesspoint)"
	$installDir/restartNetwork.sh
fi



#@@@@@@@@@@@@@@@

echo "####################################################"
echo "###                     done                     ###"
echo "####################################################"
exit 0

