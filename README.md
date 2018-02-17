lh1 - Linux HAMMER (v0.1.3)
===

## Changes

+ 2018.02.17 / v0.1.3 - Sync with DragonFly BSD.

+ 2017.10.12 / v0.1.2 - HAMMER2 sbin/newfs_hammer2 added.

+ 2017.09.30 / v0.1.1 - HAMMER1 userspace code available, except for sbin/mount_hammer. hammer info command is dropped.

## Build

        # cd lh1
        # make

## Required package for <uuid/uuid.h>

|Distribution            |Required package|
|:-----------------------|:---------------|
|Fedora                  |libuuid-devel   |
|CentOS                  |libuuid-devel   |
|Ubuntu                  |uuid-dev        |
