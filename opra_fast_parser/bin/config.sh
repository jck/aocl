#!/bin/sh

hostip0=192.168.7.43
hostip1=192.168.8.45
devip0=192.168.7.4
devip1=192.168.8.5


sudo /etc/init.d/cpuspeed stop
sudo /etc/init.d/iptables stop
sudo /sbin/ethtool -C eth2 rx-usecs 0 adaptive-rx off
sudo /sbin/onload_tool reload
sudo /sbin/onload_tool disable_cstates

sudo /sbin/ifconfig eth2 $hostip0 netmask 255.255.255.0
sudo /sbin/ifconfig eth1 $hostip1 netmask 255.255.255.0
sudo /sbin/route add $devip0 dev eth2
sudo /sbin/arp -s $devip0 55:55:AA:AA:11:11
sudo /sbin/route add $devip1 dev eth1
sudo /sbin/arp -s $devip1 55:55:AA:AA:11:12