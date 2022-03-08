#!/bin/bash

echo "Prepping Kernel"
pushd /lib/modules/5.4.0-xilinx-v2020.1/build/
sudo make scripts prepare
popd

echo "Buildling udmabuf"
unzip udmabuf-master.zip
pushd udmabuf-master
make
echo "loading udmabuf with a 1MB buffer"
sudo insmod u-dma-buf.ko udmabuf0=1048576
popd

#change the udev rules
echo "updating udev rules for userspace"
# userspace permissions for uio
sudo bash -c 'echo "KERNEL==\"uio[0-9]*\", GROUP=\"root\", MODE=\"0666\"" > /etc/udev/rules.d/99-uio.rules'

# userspace permissions for udmabuf
sudo bash -c 'echo "KERNEL==\"u-dma-buf[0-9]*\", GROUP=\"root\", MODE=\"0666\" "> /etc/udev/rules.d/99-u-dma-buf.rules'

#reload udev rules
#https://unix.stackexchange.com/questions/39370/how-to-reload-udev-rules-without-reboot
sudo bash -c 'udevadm control --reload-rules && udevadm trigger'


