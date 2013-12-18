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

echo "create installSettings programm"
cat << EOF > $installDir/installSettings.c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, const char* argv[])
{
	if (argc < 2){
		printf("There is no functionname\r\n");
		return 1;
	}
	char command[150];
	int found=0;
	if (strcmp(argv[1], "add_wlan") == 0){
		if (argc != 4){
			printf("Usage is: add_wlan ssid passphrase\r\n");
			return 1;
		}
		sprintf(command, "$installDir/add_wlan.sh \"%s\" \"%s\" 2>&1", argv[2], argv[3]);
		found=1;
	} else if (strcmp(argv[1], "restartDaemonAndMiddleware") == 0){
		if (argc != 2){
			printf("Usage is: restartDeemonAndMiddleware\r\n");
			return 1;
		}
		sprintf(command, "$installDir/restartDaemonAndMiddleware.sh 2>&1");
		found=1;
	} else if (strcmp(argv[1], "change_wlanmode") == 0){
		if (argc > 3){
			printf("Usage is: change_wlanmode [mode(toggle|ap|wpa=default)]\r\n");
			return 1;
		}
		if (argc == 3){
			sprintf(command, "$installDir/change_wlanmode.sh \"%s\" 2>&1", argv[2]);
		} else {
			sprintf(command, "$installDir/change_wlanmode.sh 2>&1");
		}
		found=1;
	} else if (strcmp(argv[1], "get_configured_wlans") == 0){
		if (argc != 2){
			printf("Usage is: get_configured_wlans\r\n");
			return 1;
		}
		sprintf(command, "$installDir/get_configured_wlans.sh 2>&1");
		found=1;
	} else if (strcmp(argv[1], "reconnect_wlan") == 0){
		if (argc != 2){
			printf("Usage is: reconnect_wlan\r\n");
			return 1;
		}
		sprintf(command, "$installDir/reconnect_wlan.sh 2>&1");
		found=1;
	} else if (strcmp(argv[1], "everycook_sync_wait") == 0){
		if (argc != 2){
			printf("Usage is: everycook_sync_wait\r\n");
			return 1;
		}
		sprintf(command, "$installDir/everycook_sync_wait.sh 2>&1");
		found=1;
	} else if (strcmp(argv[1], "restartNetwork") == 0){
		if (argc != 2){
			printf("Usage is: restartNetwork\r\n");
			return 1;
		}
		sprintf(command, "$installDir/restartNetwork.sh 2>&1");
		found=1;
	} else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "usage") == 0 || strcmp(argv[1], "-?") == 0){
		printf("Usage: %s command [params]\r\n", argv[0]);
		printf("commands are:");
		printf("\tadd_wlan ssid passphrase\r\n");
		printf("\trestartDaemonAndMiddleware\r\n");
		printf("\tchange_wlanmode [mode(toggle|ap|wpa=default)]\r\n");
		printf("\tget_configured_wlans\r\n");
		printf("\treconnect_wlan\r\n");
		printf("\trestartNetwork\r\n");
		return 0;
	}
	
	if (found == 1){
		setuid( 0 ); //run the command as root
		int result = system( command );
		
		if(result == -1) {
			printf("some error is occured in that shell command\n");
			return 1;
		/*} else if (WEXITSTATUS(result) == 127){
			printf("That shell command is not found\n");
			return 1;
		*/
		} else {
			return WEXITSTATUS(result);
		}
	} else {
		printf("unknown function %s\r\n", argv[1]);
		return 1;
	}
}
EOF
chown $userToUse:$userToUse $installDir/installSettings
chmod 0644 $installDir/installSettings

#sudo su $userToUse
pushd . > /dev/null
cd $installDir/
gcc installSettings.c -o installSettings
chown $userToUse:$userToUse installSettings*
popd > /dev/null
#sudo su
chown root:root $installDir/installSettings
chmod 4755 $installDir/installSettings


#@@@@@@@@@@@@@@@
echo "create $installDir/add_wlan.sh"
cat << EOF > $installDir/add_wlan.sh
#!/bin/sh
if [ -z "\$1" ]; then
	echo "error: param1 empty"
	exit 1;
fi
if [ -z "\$2" ]; then
	echo "error: param2 empty"
	exit 1;
fi

wlanConfigFile="/etc/wpa_supplicant/wpa_supplicant.conf"
/usr/bin/wpa_passphrase "\$1" "\$2" >> \$wlanConfigFile


$installDir/reconnect_wlan.sh noerror
EOF
chown root:root $installDir/add_wlan.sh
chmod 0750 $installDir/add_wlan.sh


#@@@@@@@@@@@@@@@



echo "create $installDir/restartDaemonAndMiddleware.sh"
cat << EOF > $installDir/restartDaemonAndMiddleware.sh
#!/bin/sh
/etc/init.d/ecdaemon stop
/etc/init.d/ecmiddleware stop
/etc/init.d/ecmiddleware start
/etc/init.d/ecdaemon start
EOF
chown $userToUse:$webuser $installDir/restartDaemonAndMiddleware.sh
chmod 0775 $installDir/restartDaemonAndMiddleware.sh


#@@@@@@@@@@@@@@@


echo "create $installDir/change_wlanmode.sh"
cat << EOF > $installDir/change_wlanmode.sh
#!/bin/bash
pushd .
curLanType=`ls -l /etc/network/interfaces | sed -n '/->/s/.* -> interfaces\(.*\)/\1/p'`
if [ "\$1" == "toggle" ]; then
	if [ "\$curLanType" == "_wlan_ap" ]; then
		lanType="_wlan_wpa"
	else
		lanType="_wlan_ap"
	fi
elseif [ "\$1" == "ap" ]; then
	lanType="_wlan_ap"
else
	lanType="_wlan_wpa"
fi

if [ "\$curLanType" != "\$lanType" ]; then
	pushd . > /dev/null
	cd /etc/network/
	rm interfaces
	ln -s interfaces\$lanType interfaces
	cd /etc/default/
	rm isc-dhcp-server
	ln -s isc-dhcp-server\$lanType isc-dhcp-server
	popd > /dev/null
	
	$installDir/restartNetwork.sh
fi
EOF
chown root:root $installDir/change_wlanmode.sh
chmod 0755 $installDir/change_wlanmode.sh

#@@@@@@@@@@@@@@@

echo "create $installDir/get_configured_wlans.sh"
cat << EOF > $installDir/get_configured_wlans.sh
#!/bin/bash
sed -n '/ssid=/s/^.*ssid="\(.*\)".*$/\1/p' /etc/wpa_supplicant/wpa_supplicant.conf
EOF
chown root:root $installDir/get_configured_wlans.sh
chmod 0755 $installDir/get_configured_wlans.sh


#@@@@@@@@@@@@@@@

echo "create $installDir/reconnect_wlan.sh"
cat << EOF > $installDir/reconnect_wlan.sh
#!/bin/bash
$checkWlanAPModeCommand
if [ "\$?" == "0" ]; then
	if [ "\$1" != "noerror" ]; then
		#if it's in AccessPoint mode it user have change to normal mode first...
		echo "error: is in AccessPoint mode, change to normal mode first"
		exit 1
	fi
else
	#$installDir/restartNetwork.sh
	/sbin/ifdown wlan0
	/sbin/ifup wlan0
fi
EOF
chown root:root $installDir/reconnect_wlan.sh
chmod 0755 $installDir/reconnect_wlan.sh


#@@@@@@@@@@@@@@@

echo "create $installDir/everycook_sync_wait.sh"
cat << EOF > $installDir/everycook_sync_wait.sh
#!/bin/bash
ping -c 1 www.everycook.org
if [ "\$?" == "0" ]; then
	pushd . > /dev/null
	cd $installDir/sync/
	./sync_privdata.sh
	./sync_recipes.sh
	./sync_php.sh
	popd > /dev/null
else
	exit 1
fi
EOF
chown root:root $installDir/everycook_sync_wait.sh
chmod 0755 $installDir/everycook_sync_wait.sh
