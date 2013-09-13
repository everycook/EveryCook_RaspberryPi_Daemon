#!/bin/bash
installDir=/opt/EveryCook
userToUse=pi
mysqlRootLogin=raspi
webuser=www-data
webmain=/var/www
webdir=$webmain/db

#checkWlanAPModeCommand="grep \"#iface wlan0 inet dhcp\" /etc/network/interfaces > /dev/null"
#checkWlanAPModeCommand="ls -l /etc/network/interfaces | sed -n '/->/s/.* -> interfaces\(.*\)/\1/p' | grep _ap"
checkWlanAPModeCommand="ls -l /etc/network/interfaces | grep _ap > /dev/null"

getWlanIPCommand="/sbin/ip -f inet addr show dev wlan0 | sed -n 's/^ *inet *\([.0-9]*\).*/\1/p'"
getLanIPCommand="/sbin/ip -f inet addr show dev eth0 | sed -n 's/^ *inet *\([.0-9]*\).*/\1/p'"