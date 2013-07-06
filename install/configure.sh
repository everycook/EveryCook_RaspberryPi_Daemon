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

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "copy hosteapd & configure /etc/hostapd/hostapd.conf"
# http://blog.sip2serve.com/post/38010690418/raspberry-pi-access-point-using-rtl8192cu
# http://dl.dropbox.com/u/1663660/hostapd/hostapd

# evtl alternative: http://www.jenssegers.be/blog/43/Realtek-RTL8188-based-access-point-on-Raspberry-Pi
# https://github.com/segersjens/RTL8188-hostapd/archive/v1.0.tar.gz

# alternative/gleiches? https://github.com/ninjablocks/rtl8192cu
# https://github.com/ninjablocks/rtl8192cu/blob/master/wpa_supplicant_hostapd/wpa_supplicant_hostapd-0.8_rtw_20120803.zip

if [ -e /usr/sbin/hostapd_orig ]; then
	#nothing
	echo "hostapd backup already exist"
else
	mv /usr/sbin/hostapd /usr/sbin/hostapd_orig
fi
cp hostapd /usr/sbin/hostapd
chmod 0755 /usr/sbin/hostapd

cat << EOF > /etc/hostapd/hostapd.conf
interface=wlan0
#driver=nl80211
driver=rtl871xdrv
ssid=EveryCook
hw_mode=g
channel=11
wpa=1
wpa_passphrase=EveryoneCanCook
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP CCMP
wpa_ptk_rekey=600
EOF
chown root:root /etc/hostapd/hostapd.conf
chmod 0644 /etc/hostapd/hostapd.conf

#@@@@@@@@@@@@@@@

echo "create /etc/default/hostapd"
cat << EOF > /etc/default/hostapd
# Defaults for hostapd initscript
#
# See /usr/share/doc/hostapd/README.Debian for information about alternative
# methods of managing hostapd.
#
# Uncomment and set DAEMON_CONF to the absolute path of a hostapd configuration
# file and hostapd will be started during system boot. An example configuration
# file can be found at /usr/share/doc/hostapd/examples/hostapd.conf.gz
#
DAEMON_CONF="/etc/hostapd/hostapd.conf"

# Additional daemon options to be appended to hostapd command:-
#       -d   show more debug messages (-dd for even more)
#       -K   include key data in debug messages
#       -t   include timestamps in some debug messages
#
# Note that -B (daemon mode) and -P (pidfile) options are automatically
# configured by the init.d script and must not be added to DAEMON_OPTS.
#
DAEMON_OPTS="-dd"
EOF
chown root:root /etc/default/hostapd
chmod 0644 /etc/default/hostapd

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "configure wlan0 dhcp /etc/dhcp/dhcpd.conf"
#add dhcp config for wlan0 to /etc/dhcp/dhcpd.conf

if [ -e /etc/dhcp/dhcpd.conf ]; then
	if [ -e /etc/dhcp/dhcpd.conf_orig ]; then
		#nothing
		echo "/etc/dhcp/dhcpd.conf backup already exist"
	else
		cp /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf_orig
	fi
	
	grep "option domain-name \"everycook-lan.local\";" /etc/dhcp/dhcpd.conf > /dev/null
	if [ "$?" != "0" ]; then
		sed '/^  range 10.0.0.10/i  option domain-name "everycook-lan.local";' /etc/dhcp/dhcpd.conf> /etc/dhcp/dhcpd.conf_temp
		mv /etc/dhcp/dhcpd.conf_temp /etc/dhcp/dhcpd.conf
	fi
	grep "subnet 10.10.0.0 netmask 255.255.255.224 {" /etc/dhcp/dhcpd.conf > /dev/null
	if [ "$?" != "0" ]; then
		sed '/^subnet 10.0.0.0/isubnet 10.10.0.0 netmask 255.255.255.224 {\n  range 10.10.0.10 10.10.0.30;\n  option domain-name "everycook-wlan.local";\n}\n' /etc/dhcp/dhcpd.conf> /etc/dhcp/dhcpd.conf_temp
		mv /etc/dhcp/dhcpd.conf_temp /etc/dhcp/dhcpd.conf
	fi
	grep "subnet 10.10.0.0 netmask 255.255.255.224 {" /etc/dhcp/dhcpd.conf > /dev/null
	if [ "$?" != "0" ]; then
		echo "" >> /etc/dhcp/dhcpd.conf
		echo "subnet 10.10.0.0 netmask 255.255.255.224 {" >> /etc/dhcp/dhcpd.conf
		echo "  range 10.10.0.10 10.10.0.30;" >> /etc/dhcp/dhcpd.conf
		echo "  option domain-name "everycook-wlan.local";" >> /etc/dhcp/dhcpd.conf
		echo "}" >> /etc/dhcp/dhcpd.conf
	fi
else
	echo "error dhcpd config (/etc/dhcp/dhcpd.conf) not yet exist..."
fi


#@@@@@@@@@@@@@@@
echo "configure wlan0 /etc/network/interfaces"
#create interface config vor normal an ap mode

if [ -e /etc/network/interfaces_wlan_ap ]; then
	#nothing
	echo "/etc/network/interfaces_wlan_ap already exist"
else
	grep -v -E "(wlan0.*dhcp)|(wpa-conf)" /etc/network/interfaces > /etc/network/interfaces_wlan_ap
	echo "" >> /etc/network/interfaces_wlan_ap
	echo "iface wlan0 inet static" >> /etc/network/interfaces_wlan_ap
	echo "address 10.10.0.1" >> /etc/network/interfaces_wlan_ap
	echo "netmask 255.255.255.0" >> /etc/network/interfaces_wlan_ap
	
	mv /etc/network/interfaces /etc/network/interfaces_wlan_wpa
	pushd . > /dev/null
	cd /etc/network/
	ln -s interfaces_wlan_ap interfaces
	popd > /dev/null
fi

#@@@@@@@@@@@@@@@
echo "configure dhcp /etc/default/isc-dhcp-server"
#create dhcp config vor normal an ap mode

#grep -v "INTERFACES=" /etc/default/isc-dhcp-server > /etc/default/isc-dhcp-server_wlan_ap
#echo "INTERFACES=\"eth0 wlan0\"" >> /etc/default/isc-dhcp-server_wlan_ap
sed '/^INTERFACES=/cINTERFACES="eth0 wlan0"' /etc/default/isc-dhcp-server > /etc/default/isc-dhcp-server_wlan_ap

mv /etc/default/isc-dhcp-server /etc/default/isc-dhcp-server_wlan_wpa
pushd . > /dev/null
cd /etc/default/
ln -s isc-dhcp-server_wlan_ap isc-dhcp-server
popd > /dev/null

#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "create /etc/apache2/sites-available/everycook"
#create and link everycook apache site config 

cat << EOF > /etc/apache2/sites-available/everycook
<VirtualHost *:80>
        ServerAdmin webmaster@localhost

        DocumentRoot $webmain
        <Directory />
                Options FollowSymLinks
                AllowOverride None
        </Directory>
        <Directory $webmain/>
                Options FollowSymLinks MultiViews
                AllowOverride None
                Order allow,deny
                allow from all
        </Directory>

        <Directory $webdir>
                AllowOverride FileInfo
        </Directory>

        ScriptAlias /cgi-bin/ /usr/lib/cgi-bin/
        <Directory "/usr/lib/cgi-bin">
                AllowOverride None
                Options +ExecCGI -MultiViews +SymLinksIfOwnerMatch
                Order allow,deny
                Allow from all
        </Directory>

        ErrorLog \${APACHE_LOG_DIR}/error.log

        # Possible values include: debug, info, notice, warn, error, crit,
        # alert, emerg.
        LogLevel warn

        CustomLog \${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
EOF
chown root:root /etc/apache2/sites-available/everycook
chmod 0644 /etc/apache2/sites-available/everycook

rm /etc/apache2/sites-enabled/000-default
pushd . > /dev/null
cd /etc/apache2/sites-enabled/
ln -s ../sites-available/everycook 000-everycook
popd > /dev/null

pushd . > /dev/null
cd /etc/apache2/mods-enabled/
ln -s ../mods-available/rewrite.load rewrite.load
popd > /dev/null


#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "create /etc/init.d/ecdaemon"
cat << EOF > /etc/init.d/ecdaemon
#!/bin/bash
# /etc/init.d/ecdaemon

### BEGIN INIT INFO
# Provides:          ecdaemon
# Short-Description: Start EveryCook daemon
# Required-Start:    \$all
# Required-Stop:     \$all
# Should-Start:
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO

server_ip=\`$installDir/getServerIp.sh\`
#server_ip=\`/opt/EveryCook/getServerIp.sh\`
server_ip=127.0.0.1
daemonPath=/opt/EveryCook/daemon
#daemonCommand="\${daemonPath}/ecdaemon --config \${daemonPath}/config --middleware-and-file --middleware-server \${server_ip} --sim --sim7seg"
daemonCommand="\${daemonPath}/ecdaemon --config \${daemonPath}/config --middleware-and-file --middleware-server \${server_ip}"

# Some things that run always
# Carry out specific functions when asked to by the system
case "\$1" in
  restart)
    #stop
    if [ -f \${daemonPath}/ecdaemon.pid ]; then
        ps -e|grep \`cat \${daemonPath}/ecdaemon.pid\` > /dev/null
        if [[ \$? -eq 0 ]]; then
			echo "Stopping ecdaemon"
			kill -15 \`cat \${daemonPath}/ecdaemon.pid\`
			sleep 3
			ps -e|grep \`cat \${daemonPath}/ecdaemon.pid\` > /dev/null
			if [[ \$? -eq 0 ]]; then
			    kill -9 \`cat \${daemonPath}/ecdaemon.pid\`
			fi
			#rm \${daemonPath}/ecdaemon.pid
        fi
    fi
    #start
    ps -e|grep \`cat \${daemonPath}/ecdaemon.pid\` > /dev/null
    if [[ \$? -eq 0 ]]; then
        echo "ecdaemon still running, stop not successfull"
        exit 2
    fi
    echo "Starting ecdaemon"
    \${daemonCommand}  > \${daemonPath}/ecdaemon.log 2>&1 &
    echo -ne \$! > \${daemonPath}/ecdaemon.pid
    ;;
  start)
    if [ -f \${daemonPath}/ecdaemon.pid ]; then
        ps -e|grep \`cat \${daemonPath}/ecdaemon.pid\` > /dev/null
        if [[ \$? -eq 0 ]]; then
            echo "ecdaemon already started"
            exit 2
        fi
    fi
    echo "Starting ecdaemon"
    \${daemonCommand}  > \${daemonPath}/ecdaemon.log 2>&1 &
    echo -ne \$! > \${daemonPath}/ecdaemon.pid
    ;;
  stop)
    if [ -f \${daemonPath}/ecdaemon.pid ]; then
        ps -e|grep \`cat \${daemonPath}/ecdaemon.pid\` > /dev/null
        if [[ \$? -ne 0 ]]; then
            echo "ecdaemon already stoped"
            exit 2
        fi
        echo "Stopping ecdaemon"
        kill -15 \`cat \${daemonPath}/ecdaemon.pid\`
        sleep 3
        ps -e|grep \`cat \${daemonPath}/ecdaemon.pid\` > /dev/null
        if [[ \$? -eq 0 ]]; then
            kill -9 \`cat \${daemonPath}/ecdaemon.pid\`
        fi
        #rm \${daemonPath}/ecdaemon.pid
    else
        echo "pid file not found..."
        exit 3
    fi
    ;;
  *)
    echo "Usage: /etc/init.d/ecdaemon {start|stop}"
    exit 1
    ;;
esac

exit 0
EOF
chown root:root /etc/init.d/ecdaemon
chmod 0755 /etc/init.d/ecdaemon

/usr/sbin/update-rc.d ecdaemon defaults

#@@@@@@@@@@@@@@@
echo "create /etc/init.d/ecmiddleware"
cat << EOF > /etc/init.d/ecmiddleware
#!/bin/bash
# /etc/init.d/ecmiddleware

### BEGIN INIT INFO
# Provides:          ecmiddleware
# Short-Description: Start EveryCook middleware
# Required-Start:    \$all
# Required-Stop:     \$all
# Should-Start:
# Should-Stop:
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
### END INIT INFO

middlewarePath=$installDir/middleware
middlewareCommand="php -f \${middlewarePath}/startdaemon.php"

# Some things that run always

# Carry out specific functions when asked to by the system
case "\$1" in
  restart)
    #stop
    echo "Stopping ecmiddleware"
    kill -9 \`cat \${middlewarePath}/middleware.pid\`
	#start
    echo "Starting ecmiddleware "
    \${middlewareCommand} > \${middlewarePath}/ecmiddleware.log 2>&1 &
	echo -ne \$! > \${middlewarePath}/middleware.pid
    ;;
  start)
    echo "Starting ecmiddleware "
    \${middlewareCommand} > \${middlewarePath}/ecmiddleware.log 2>&1 &
	echo -ne \$! > \${middlewarePath}/middleware.pid
    ;;
  stop)
    echo "Stopping ecmiddleware"
    kill -9 \`cat \${middlewarePath}/middleware.pid\`
#	sleep 1
#	kill -15 \`cat \${middlewarePath}/middleware.pid\`
    ;;
  *)
    echo "Usage: /etc/init.d/ecmiddleware {start|stop}"
    exit 1
    ;;
esac

exit 0
EOF
chown root:root /etc/init.d/ecmiddleware
chmod 0755 /etc/init.d/ecmiddleware

/usr/sbin/update-rc.d ecmiddleware defaults


#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "create /etc/network/if-up.d/everycook_sync"
#create handler if network goeas up -> sync everycook files

cat << EOF > /etc/network/if-up.d/everycook_sync
#!/bin/bash
ping -c 1 www.everycook.org
if [ "\$?" == "0" ]; then
	pushd . > /dev/null
	cd $installDir/sync/
	./sync_php.sh &
	./sync_recipes.sh &
	./sync_privdata.sh &
	popd > /dev/null
fi
EOF
chown root:root /etc/network/if-up.d/everycook_sync
chmod 0755 /etc/network/if-up.d/everycook_sync



#@@@@@@@@@@@@@@@
echo "create $installDir/restartNetwork.sh"

cat << EOF > $installDir/restartNetwork.sh
#!/bin/bash
wlan_ip_before=\`$getWlanIPCommand\`

$checkWlanAPModeCommand
if [ "\$?" == "0" ]; then
	echo "is in AccessPoint mode..."
	lanMode="ap"
else
	echo "is in normal mode..."
	lanMode="wpa"
fi

echo "stop hostapd, dhcpd, bring down wlan0"
/etc/init.d/hostapd stop
/etc/init.d/isc-dhcp-server stop
/sbin/ifdown wlan0

if [ "\$lanMode" == "ap" ]; then
	echo "bring up wlan0 again, start dhcpd and hostapd"
else
	echo "bring up wlan0 again, start dhcpd (but not hostapd)"
fi

/sbin/ifup wlan0
/etc/init.d/isc-dhcp-server start
if [ "\$lanMode" == "ap" ]; then
	/etc/init.d/hostapd start
fi


wlan_ip=\`$getWlanIPCommand\`


if [ "\$wlan_ip_before" !=  "\$wlan_ip" ]; then
	#restart daemon & middleware with new ip
	$installDir/restartDaemonAndMiddleware.sh
fi
if [ -n "\$wlan_ip" ]; then
	echo \$wlan_ip
else
	echo "error: wlan has no ip"
fi
EOF
chown root:root $installDir/restartNetwork.sh
chmod 0755 $installDir/restartNetwork.sh

#@@@@@@@@@@@@@@@
echo "create $installDir/checkWlanHasIp.sh"
#add wlan0 has ip check to /etc/rc.local

cat << EOF > $installDir/checkWlanHasIp.sh
#!/bin/bash
echo "only check if it's in Access-Point mode"
$checkWlanAPModeCommand
if [ "\$?" == "0" ]; then
	echo "check wlan has ip..."
	wlan_ip=\`$getWlanIPCommand\`
	if [ -z "\$wlan_ip" ]; then
		$installDir/restartNetwork.sh
	fi
#else
#	/etc/init.d/hostapd stop
fi
EOF
chown root:root $installDir/checkWlanHasIp.sh
chmod 0755 $installDir/checkWlanHasIp.sh

grep "checkWlanHasIp.sh" /etc/rc.local > /dev/null
if [ "$?" != "0" ]; then
	sed '/^# Print the IP address/i\/opt\/EveryCook\/checkWlanHasIp.sh\n/' /etc/rc.local> /etc/rc.local
fi
grep "checkWlanHasIp.sh" /etc/rc.local > /dev/null
if [ "$?" != "0" ]; then
	sed '/^exit 0/i\/opt\/EveryCook\/checkWlanHasIp.sh\n/' /etc/rc.local> /etc/rc.local
fi
grep "checkWlanHasIp.sh" /etc/rc.local > /dev/null
if [ "$?" != "0" ]; then
	echo "" >> /etc/rc.local
	echo "$installDir/checkWlanHasIp.sh" >> /etc/rc.local
fi


#@@@@@@@@@@@@@@@
echo "create $installDir/getServerIp.sh"
cat << EOF > $installDir/getServerIp.sh
#!/bin/sh
wlan_ip=\`$getWlanIPCommand\`;
lan_ip=\`$getLanIPCommand\`;

server_ip='';

if [ -n "\$wlan_ip" ]; then
	server_ip=\$wlan_ip
else
	if [ -n "\$lan_ip" ]; then
		server_ip=\$lan_ip
	else
		server_ip='10.0.0.1'
	fi
fi

if [ -z "\$server_ip" ]; then
	server_ip='10.0.0.1'
fi

echo \$server_ip
EOF
chown $userToUse:$webuser $installDir/getServerIp.sh
chmod 0775 $installDir/getServerIp.sh


#@@@@@@@@@@@@@@@
echo "create $installDir/getAllIps.sh"
cat << EOF > $installDir/getAllIps.sh
#!/bin/sh
/sbin/ip -f inet addr show | sed -n 's/^ *inet *\([.0-9]*\).*/\1/p'
EOF
chown $userToUse:$webuser $installDir/getAllIps.sh
chmod 0775 $installDir/getAllIps.sh


#@@@@@@@@@@@@@@@

echo "create $installDir/check_wlan_connected.sh"
cat << EOF > $installDir/check_wlan_connected.sh
#!/bin/bash
$checkWlanAPModeCommand
if [ "\$?" == "0" ]; then
	wlan_ip=\`$getWlanIPCommand\`;
	if [ -z "\$wlan_ip" ]; then
		$installDir/restartNetwork.sh
		exit 1;
	fi
	#TODO: check if dhcp server is up and running, else restart network also
else
	wlan_ip=\`$getWlanIPCommand\`;
	if [ -z "\$wlan_ip" ]; then
		/sbin/ifup wlan0
		exit 1;
	fi
fi
exit 0;
EOF
chown root:root $installDir/check_wlan_connected.sh
chmod 0755 $installDir/check_wlan_connected.sh


#@@@@@@@@@@@@@@@
echo "create $installDir/backupConfig.sh"
cat << EOF > $installDir/backupConfig.sh
#!/bin/sh

if [ ! -d '/home/pi/backups' ]; then
	mkdir /home/pi/backups
fi

destname=\`date +"%Y%m%d_%H%M"\`
destname1=/home/pi/backups/config_\$destname
destname2=/home/pi/backups/calibration_\$destname
cp $installDir/daemon/config \$destname1
cp $installDir/daemon/calibration \$destname2
EOF
chown $userToUse:$webuser $installDir/backupConfig.sh
chmod 0775 $installDir/backupConfig.sh


#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "install cron every 5 min for 'sudo $installDir/check_wlan_connected.sh'"
crontab - << EOF
*/5 * * * * sudo $installDir/check_wlan_connected.sh >/dev/null 2>&1
EOF

