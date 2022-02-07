## Reference

https://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/

## Sequence

* Enable debug in Kernel (ensure the following is set in .config):
  * GDB_SCRIPTS = y
  * DEBUG_INFO = y
  * DEBUG_INFO_REDUCED = n
*  In MKT: launch with "--gdbserver 1234" option
*  Start GDB with vmlinux binary
  *  If the following warning is observed, do as advised:
```
$ gdb vmlinux                                                                                                                                                 
GNU gdb (GDB) Fedora 10.2-4.fc33
...
Reading symbols from vmlinux...
warning: File "<kernel-path>/scripts/gdb/vmlinux-gdb.py" auto-loading has been declined by your `auto-load safe-path' set to "$debugdir:$datadir/auto-load:/images/artemp/src/kernel/scripts/gdb/vmlinux-gdb.py".
To enable execution of this file add
        add-auto-load-safe-path <kernel-path>/scripts/gdb/vmlinux-gdb.py
line to your configuration file "~/.gdbinit".
To completely disable this security protection add
        set auto-load safe-path /
line to your configuration file "~/.gdbinit".
For more information about this security protection see the
"Auto-loading safe path" section in the GDB manual.  E.g., run from the shell:
        info "(gdb)Auto-loading safe path"
```
* Attach to the running Kernel
```
(gdb) target remote :1234
```
