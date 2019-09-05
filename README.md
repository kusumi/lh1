lh1 - Linux HAMMER (v0.1.8)
===

## About

+ HAMMER1/2 userspace utilities on Linux / x86_64.

## Changes

+ 2019.09.05 / v0.1.8 - Sync with DragonFly BSD.

+ 2019.08.20 / v0.1.7 - Sync with DragonFly BSD.

+ 2019.01.13 / v0.1.6 - Sync with DragonFly BSD.

+ 2018.07.10 / v0.1.5 - HAMMER2 sbin/hammer2 added (not fully functional).

+ 2018.06.17 / v0.1.4 - Sync with DragonFly BSD.

+ 2018.02.17 / v0.1.3 - Sync with DragonFly BSD.

+ 2017.10.12 / v0.1.2 - HAMMER2 sbin/newfs_hammer2 added.

+ 2017.09.30 / v0.1.1 - HAMMER1 userspace code available, except for sbin/mount_hammer. hammer info command is dropped.

## Commands

|Command         |Description                                     |
|:---------------|:-----------------------------------------------|
|hammer(8)       |HAMMER filesystem utility                       |
|newfs_hammer(8) |construct a new HAMMER filesystem               |
|mount_hammer(8) |mount a HAMMER file system                      |
|hammer2(8)      |HAMMER2 filesystem utility                      |
|newfs_hammer2(8)|construct a new HAMMER2 filesystem              |
|mount_hammer2(8)|mount a HAMMER2 file system                     |
|undo(1)         |undo changes made to files on HAMMER filesystems|
|fstyp(8)        |determine filesystem type                       |

## Build

        # cd lh1
        # make

### Required package for <uuid/uuid.h>

|Distribution|Required package|
|:-----------|:---------------|
|Fedora      |libuuid-devel   |
|RHEL        |libuuid-devel   |
|Ubuntu      |uuid-dev        |

### Required package for <openssl/*.h>

|Distribution|Required package|
|:-----------|:---------------|
|Fedora      |openssl-devel   |
|RHEL        |openssl-devel   |
|Ubuntu      |libssl-dev      |

## Install

        # cd lh1
        # make && make install

## Uninstall

        # cd lh1
        # make uninstall
