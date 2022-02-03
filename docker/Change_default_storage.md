## Ref
https://linuxconfig.org/how-to-move-docker-s-default-var-lib-docker-to-another-directory-on-ubuntu-debian-linux

## Stop the Docker service
```
[root@example.host RDMO]# systemctl stop docker   
Warning: Stopping docker.service, but it can still be activated by:
  docker.socket
[root@example.host RDMO]# systemctl stop docker.socket
```

## Move all the files from `var/lib/docker` to the new directory
```
[root@example.host RDMO]# ls /var/lib/docker
buildkit  containers  image  network  overlay2  plugins  runtimes  swarm  tmp  trust  volumes
[root@example.host RDMO]# rsync -aqxP /var/lib/docker/ /images/docker/
[root@example.host RDMO]# ls /images/docker/
buildkit  containers  image  network  overlay2  plugins  runtimes  swarm  tmp  trust  volumes
[root@example.host RDMO]# rm -Rf /var/lib/docker/*
```

## Modify the docker service description specifying the new path
```
[root@example.host RDMO]# sudo nano /lib/systemd/system/docker.service
[Unit]
Description=Docker Application Container Engine
Documentation=https://docs.docker.com
After=network-online.target firewalld.service containerd.service
Wants=network-online.target
Requires=docker.socket containerd.service

[Service]
Type=notify
# the default is not to use systemd for cgroups because the delegate issues still
# exists and systemd currently does not support the cgroup feature set required
# for containers run by docker
ExecStart=/usr/bin/dockerd -H fd:// --containerd=/run/containerd/containerd.sock # <<<<<<<< !!!!!
```

Modify to:
```
[root@example.host RDMO]# sudo nano /lib/systemd/system/docker.service
ExecStart=/usr/bin/dockerd -H fd:// --containerd=/run/containerd/containerd.sock -g /images/docker
```

## Restart docker
```
[root@example.host RDMO]# systemctl daemon-reload
[root@example.host RDMO]# systemctl start docker 
```

## Enable Docker by default
```
[root@example.host RDMO]# systemctl enable docker 
```
