#!/bin/sh

tar xvfz bcm2835-1.8.tar.gz;
cd bcm2835-1.8;
./configure;
make;
sudo make install;
cd ..

mkdir spi
cp new_spi spi/
cat << EOF > spi/compileAndRun.sh
gcc -o new_spi -I ../bcm2835-1.8/src ../bcm2835-1.8/src/bcm2835.c new_spi.c
sudo ./new_spi
EOF
chmod 775 spi/compileAndRun.sh

echo done.
