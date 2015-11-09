#!/bin/bash

#Exp1---- Configure IP Address for tux41
ifconfig eth0 up
ifconfig eth0 172.16.40.1/24
ifconfig

# Verify connection to tux44
# ping 172.16.40.254

# Inspect forwarding
# route -n

# Inspect ARP Tables
# arp -a

# Delete ARP entries
# arp -d 172.16.40.254


#Exp2----all in gtkterm

#Exp3

route add -net 172.16.41.0/24 gw 172.16.40.254
#route -n 
 
#Exp4

route add default gw 172.16.40.254
