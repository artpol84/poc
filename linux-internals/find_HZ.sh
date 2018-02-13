awk '{print$22/'$(tail -n 1 /proc/uptime|cut -d. -f1)"}" /proc/self/stat
