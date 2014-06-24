file="firmware.hex"
if [ -e "$file" ] && [ -s "$file" ]
then
  gpio -g mode 4 out
  gpio -g write 4 1
  sudo avrdude -F -V -c gpio -p m644p -P gpio -b 57600 -U flash:w:$file
  gpio -g write 4 0
else
  echo "$file does not exist or is empty"
  exit 1
fi
