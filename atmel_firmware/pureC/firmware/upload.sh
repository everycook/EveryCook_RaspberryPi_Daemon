sudo avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:firmware.hex
