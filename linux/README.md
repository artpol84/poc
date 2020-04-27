## Useful tips

### Query all directories from the particular mount point:
```bash
$ for i in `ls -1 <path>`; do 
  mp=`findmnt -n -o SOURCE --target $i`;
  if [ "$mp" = <mount-point> ]; then 
    echo "Process $i";
    sudo du -hs $i;
  fi
done
```
