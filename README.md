lh1 - Linux HAMMER (v0.1.14)
===

## About

HAMMER1/2 userspace utilities on Linux / x86_64

## Commands

|Command         |Description                                     |
|:---------------|:-----------------------------------------------|
|hammer(8)       |HAMMER filesystem utility                       |
|newfs_hammer(8) |construct a new HAMMER filesystem               |
|mount_hammer(8) |mount a HAMMER file system                      |
|hammer2(8)      |HAMMER2 filesystem utility                      |
|newfs_hammer2(8)|construct a new HAMMER2 filesystem              |
|mount_hammer2(8)|mount a HAMMER2 file system                     |
|fsck_hammer2(8) |HAMMER2 file system consistency checker         |
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
