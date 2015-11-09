#!/bin/bash

#Exp1

ifconfig eth0 up
ifconfig eth0 172.16.41.1/24
#ifconfig

#Exp3

route add -net 172.16.40.0/24 gw 172.16.41.253

#route -n

#Exp4


route add default gw 172.16.41.254
