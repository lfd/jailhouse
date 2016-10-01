#!/bin/bash

JAPTER_VERSION=V11/japter-safe
CONFIG=./configs/jetson-tk1-linux-demo.cell
ZIMAGE=~/zImage-inmate
INITRD=~/rootfs.cpio
DTB=./configs/dts/inmate-jetson-tk1.dtb
REMOTE_IP="192.168.178.10"

CMDLINE="console=ttyS0,115200 japter=${JAPTER_VERSION} remote_ip=${REMOTE_IP}"

./tools/jailhouse-cell-linux \
	./configs/jetson-tk1-linux-demo.cell \
	${ZIMAGE} \
	--dtb ${DTB} \
	--cmdline "${CMDLINE}" \
	--initrd ${INITRD}
