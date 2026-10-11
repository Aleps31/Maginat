#!/bin/bash
LFS=/mnt/lfs
mountpoint -q $LFS/dev/shm && umount $LFS/dev/shm
umount $LFS/dev/pts $LFS/sys $LFS/proc $LFS/run $LFS/dev
mount | grep /mnt/lfs
