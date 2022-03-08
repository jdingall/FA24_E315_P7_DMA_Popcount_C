#!/bin/bash -x

BITBIN=bd_fpga_wrapper.bit.bin 

#copy to /lib/firmware folder
sudo bash -c "cp $BITBIN /lib/firmware/custom.bit.bin"
# set total reporgramming of bitstream
sudo bash -c "echo \"0\" >>  /sys/class/fpga_manager/fpga0/flags"
# tell fpga_manager to load new bitstream
sudo bash -c "echo \"custom.bit.bin\" >> /sys/class/fpga_manager/fpga0/firmware"

# double-check everything went ok
dmesg | tail -n1

