## Run the set of measurements

```
$ hdr_opt=""; i=8; while [ "$i" -lt $((512*1024*1024)) ]; do ./mem_subs -t cache:strided -m "m:1:16" -p 2 -s $i $hdr_opt; i=$(( $i * 2 )); hdr_opt="-q"; done
```
