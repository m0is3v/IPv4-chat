#!/bin/bash

if [ $# -ne 3 ]; then
    echo "Usage: $0 <interface> <ip_address> <netmask>"
    echo "Example: $0 wlo1 192.168.1.100 255.255.255.0"
    exit 1
fi

INTERFACE=$1
IP_ADDR=$2
NETMASK=$3

if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root" >&2
    exit 1
fi

echo "Configuring network interface $INTERFACE..."
ip addr add $IP_ADDR/$NETMASK dev $INTERFACE
ip link set $INTERFACE up

if [ $? -eq 0 ]; then
    echo "Network configured successfully:"
    ip addr show $INTERFACE
else
    echo "Failed to configure network" >&2
    exit 1
fi
