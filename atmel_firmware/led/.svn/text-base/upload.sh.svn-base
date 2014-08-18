file="led.hex"
if [ -e "$file" ] && [ -s "$file" ]
then
  sudo /etc/init.d/ecdaemon stop
  stopDeamonStatus=$?
  gpio -g mode 17 out
  gpio -g write 17 1
  sudo avrdude -F -V -c gpio -p m644 -P gpio -b 57600 -U flash:w:$file
  gpio -g write 17 0
  if [ $stopDeamonStatus == 0 ]
  then
    sudo /etc/init.d/ecdaemon start
  fi
else
  echo "$file does not exist or is empty"
  exit 1
fi
