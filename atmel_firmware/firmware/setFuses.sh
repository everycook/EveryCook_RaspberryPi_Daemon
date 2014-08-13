gpio -g mode 17 out
gpio -g write 17 1
sleep 0.5
sudo avrdude -P gpio -c gpio -p m644 -U lfuse:w:0xce:m -U hfuse:w:0x99:m -U efuse:w:0xff:m
#8mhz startup 16k+65ms
#sudo avrdude -P gpio -c gpio -p m644 -U lfuse:w:0xf9:m -U hfuse:w:0xDC:m -U efuse:w:0xFD:m
gpio -g write 17 0
