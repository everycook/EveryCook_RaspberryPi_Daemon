if /sbin/ifconfig eth0 | /bin/grep 10.0.0 2>&1 > /dev/null; then
echo "all good we have an IP on eth0";
else
echo "no IP, restarting eth0 and dhcp-server";
/sbin/ifdown eth0
/sbin/ifup eth0
/etc/init.d/isc-dhcp-server restart
echo "done"
fi
