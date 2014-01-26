gpio -g write 4 1
sudo avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:firmware.hex
gpio -g write 4 0
