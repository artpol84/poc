## Rocky Linux (likely the same for RedHat & CentOS)

#### List available repositories
```
[root@thor036 ~]# dnf repolist --all
repo id                     repo name                                 status
appstream                   Rocky Linux 8 - AppStream                 enabled
appstream-debug             Rocky Linux 8 - AppStream - Source        disabled
appstream-source            Rocky Linux 8 - AppStream - Source        disabled
baseos                      Rocky Linux 8 - BaseOS                    enabled
....
```

#### Enable/Disable availale repositories
```
[root@thor036 ~]# dnf repolist --enablerepo="epel*,power*,baseos*,devel" --all
repo id                     repo name                                 status
appstream                   Rocky Linux 8 - AppStream                 enabled
appstream-debug             Rocky Linux 8 - AppStream - Source        disabled
appstream-source            Rocky Linux 8 - AppStream - Source        disabled
baseos                      Rocky Linux 8 - BaseOS                    enabled
baseos-debug                Rocky Linux 8 - BaseOS - Source           enabled
````


### Install debug info for GlibC
#### GDB provides the hint when attaching to the process:
```
yum debuginfo-install \
     glibc-2.28-164.el8_5.3.x86_64 libgcc-8.5.0-4.el8_5.x86_64 libmount-2.32.1-28.el8.x86_64 libnl3-3.5.0-1.el8.x86_64 \
     libpciaccess-0.14-1.el8.x86_64 libselinux-2.9-5.el8.x86_64 libuuid-2.32.1-28.el8.x86_64 libxml2-2.9.7-12.el8_5.x86_64 \
     pcre2-10.32-2.el8.x86_64 systemd-libs-239-51.el8_5.5.x86_64 xz-libs-5.2.4-3.el8.1.x86_64 zlib-1.2.11-17.el8.x86_64
```

#### On Rocky Linux 8.5, to enable `debuginfo` additional repos must be enabled
```
yum debuginfo-install --enablerepo="epel*,power*,baseos*,devel" \
     glibc-2.28-164.el8_5.3.x86_64 libgcc-8.5.0-4.el8_5.x86_64 libmount-2.32.1-28.el8.x86_64 libnl3-3.5.0-1.el8.x86_64 \
     libpciaccess-0.14-1.el8.x86_64 libselinux-2.9-5.el8.x86_64 libuuid-2.32.1-28.el8.x86_64 libxml2-2.9.7-12.el8_5.x86_64 \
     pcre2-10.32-2.el8.x86_64 systemd-libs-239-51.el8_5.5.x86_64 xz-libs-5.2.4-3.el8.1.x86_64 zlib-1.2.11-17.el8.x86_64
```
