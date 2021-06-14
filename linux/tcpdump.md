## TCPDUMP examples

### Monitor DHCP traffic
```
# tcpdump -i eth0 port 67 or port 68 -e -n -vv
```

### Filter by MAC address
```
# All traffic to/from host
tcpdump -i eth0 -env ether host 00:AA:BB:CC:DD:EE
# All traffic from host
tcpdump -i eth0 -env ether src 00:AA:BB:CC:DD:EE
# All traffic to host
tcpdump -i eth0 -env ether dst 00:AA:BB:CC:DD:EE
```