lh1 - Linux HAMMER (v0.1.5)
===

## About

+ HAMMER1/2 userspace utilities on Linux / x86_64.

## Changes

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
|hammer2(8)      |HAMMER2 filesystem utility                      |
|newfs_hammer2(8)|construct a new HAMMER2 filesystem              |
|undo(1)         |undo changes made to files on HAMMER filesystems|
|fstyp(8)        |determine filesystem type                       |

## Build

        # cd lh1
        # make

### Required package for <uuid/uuid.h>

|Distribution|Required package|
|:-----------|:---------------|
|Fedora      |libuuid-devel   |
|CentOS      |libuuid-devel   |
|Ubuntu      |uuid-dev        |

### Required package for <openssl/*.h>

|Distribution|Required package|
|:-----------|:---------------|
|Fedora      |openssl-devel   |
|CentOS      |openssl-devel   |
|Ubuntu      |libssl-dev      |

## Install

        # cd lh1
        # make && make install

## Uninstall

        # cd lh1
        # make uninstall
