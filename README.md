#EveryCook_RaspberryPi_Daemon

A Daemon, written in C to read the EveryCook sensors and control the actors.

##Installation Guide to get this running on your Raspberry Pi

For this guide we used the raspbian image from  
http://www.raspberrypi.org/downloads  
in version 2013-07-26-wheezy-raspbian.zip  
(http://downloads.raspberrypi.org/raspbian_latest)  

First get your raspbian running headless (without monitor)
* copy the image above on a 4GB (or greater) SD-Card.
* Start the raspberry pi with the SD-Card and connected display an keyboard.
* on first start the "raspi-config" is started, here you can expand filesystem, change locale, timezone, passwort for "pi" (raspberry for default).
* you need to expand the filesystem to have enough space to install all software and data you need, if you only have a 2GB SD-card you have to uninstall unused software (see later steps for it)
* after setting the change the configs reboot it will ask to (or "sudo reboot")
* from now on, you can connect the raspberryPi over ssh

Then to get the needed packages type:

**sudo apt-get update
sudo apt-get install apache2 espeak i2c-tools mysql-server php5-cli php5-mysql php5-gd libapache2-mod-php5filter memcached php5-memcached php-apc watchdog isc-dhcp-server hostapd libutempter0 screen**

this should install the packages:
apache2 apache2-mpm-prefork apache2-utils apache2.2-bin apache2.2-common espeak espeak-data heirloom-mailx hostapd i2c-tools isc-dhcp-server libaio1 libapache2-mod-php5filter libapr1 libaprutil1 libaprutil1-dbd-sqlite3 libaprutil1-ldap libdbd-mysql-perl libdbi-perl libespeak1 libhtml-template-perl libjack-jackd2-0 libmemcached10 libmysqlclient16 libmysqlclient18 libnet-daemon-perl libonig2 libplrpc-perl libportaudio2 libqdbm14 libsonic0 memcached mysql-client-5.5 mysql-common mysql-server mysql-server-5.5 mysql-server-core-5.5 php-apc php5-cli php5-common php5-gd php5-memcached php5-mysql ssl-cert watchdog libutempter0 screen

while installing you need to set the mysql root password and remember it for later


Now to download and prepare the everycook software:

**cd /home/pi  
mkdir codes  
cd codes**

checkout daemon git repository
**git clone git://github.com/everycook/EveryCook_RaspberryPi_Daemon.git /home/pi/codes**

checkout wiringPi library from original author
**git clone git://git.drogon.net/wiringPi /home/pi/wiringPi**

install the wiring pi library
**cd /home/pi/wiringPi  
sudo ./build**


make everycook daemon  
*cd /home/pi/codes/daemon
make*


change install settings  
goto install directory  
*cd /home/pi/codes/install*

In file "intallVars.sh" set the "mysqlRootLogin" to the passwort you defined while installation of mysql-server.
you can also change the installation directory(installDir) or the users that should use for file owners(userToUse / webuser)

run everycook install

**sudo su  
chmod 0775 *.sh  
chown pi:pi *.sh**

all packeages are installed by apt-get already so no need to install them by install script (therefore the nopackages param), also do not restart network, so chanegs from install script will not effect allready (norestart param)
**./runInstall.sh nopackages norestart**

exit su mode
**exit**

change settings of everycook daemon

now open the RaspberryPI in you browser http://raspberrypi/ (or with ip of your raspi http://192.168.1.XXX)
you will redirected to http://raspberrypi/install.php here you have to set the nickname and password of your user on everycook.org, so the system can sync your private data.

you can here also add your local wlan ssid's and password to connect on. (see available lans here)
and you can change the wlan to normal mode (currently its in AccessPoint mode)


alternatively you can add your user and password in file
/opt/Everycook/sync/login_cred
user=[username]&pw=[password]

to add a wlan you can also use the command 
**sudo wpa_passphrase [wlan-ssid] [pw] >> /etc/wpa_supplicant/wpa_supplicant.conf**

sync page/recipes/private data from everycook.org

**sudo /opt/EveryCook/everycook_sync_wait.sh**

you should get a output like this  
**PING www.everycook.org (83.169.34.51) 56(84) bytes of data.
64 bytes from everycook.org (83.169.34.51): icmp_req=1 ttl=49 time=136 ms

--- www.everycook.org ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 136.897/136.897/136.897/0.000 ms
import priv data
files synced
import changes
mkdir: kann Verzeichnis „/var/www/db/assets“ nicht anlegen: Die Datei existiert bereits
mkdir: kann Verzeichnis „/var/www/db/cache“ nicht anlegen: Die Datei existiert bereits
mkdir: kann Verzeichnis „/var/www/db/protected/runtime“ nicht anlegen: Die Datei existiert bereits
mkdir: kann Verzeichnis „/var/www/db/protected/config“ nicht anlegen: Die Datei existiert bereits
files synced**

if you get a error like "ERROR: Incorrect date and time argument: 00:00:00" run the script again.

Instead of "import changes" there could also be "import full mysqldump" and "import changes since mysqldump".

If you have the message "Error getting private data." in the log, you have to set the logindata as described above.
After fixing the login rerun the script

You can also start this script from the the EveryCook interface using "jump to" -> "sync from platform" 

The you need to activate the i2c and spi modules of your system:

comment out the lines  
**blacklist spi-bcm2708
blacklist i2c-bcm2708**

**/etc/modprobe.d/raspi-blacklist.conf**

add the lines  
**i2c-dev  
w1-gpio  
w1-therm**  
to
**/etc/modules**


Prepare the Serial Interface for connecting Arduino/Atmel:
Disable login screen:
find this line in file /etc/inittab
**T0:23:respawn:/sbin/getty -L ttyAMA0 115200 vt100**
and comment it out.

don't send boot mesage to serial:
remove
**console=ttyAMA0,115200 kgdboc=ttyAMA0,115200**
from /boot/cmdline.txt


install minicon:
**sudo apt-get install minicon**

add udev rule for map ttyAMA0 to ttyUSB9:
add
**KERNEL=="ttyAMA0",SYMLINK+="ttyUSB9" GROUP="dialout"**
to
/etc/udev/rules.d/99-input.rules

reboot system

now the installation is finished, so reboot the system to activate all changes to configs made.
after the reboot the network is configured as fix ip 10.0.0.1 and don't use dhcp anymore. You can change this back in /etc/network/interfaces (a symlink to /etc/network/interfaces_wlan_ap or /etc/network/interfaces_wlan_wpa)

if you have added a wlan-stick, and the driver for it is installed, you can access the raspberryPI also with WLAN as an accesspoint.
The WLan ssid is "everycook" the password is "EveryoneCanCook". the ip of the raspi is then 10.10.0.1

Create Issues if something does not work as expected.
