killall ecdaemon -9
killall ecdaemon -9
sleep 1
echo "daemon killed, setting fuses"
gpio -g mode 17 out
gpio -g write 17 1
sleep 0.5
sudo avrdude -P gpio -c gpio -p m644 -U lfuse:w:0xce:m -U hfuse:w:0x99:m -U efuse:w:0xff:m
gpio -g write 17 0
