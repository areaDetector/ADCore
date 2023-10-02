#!/bin/sh
set -e -x

[ -d "asyn/asynDriver" ] || exit 1

[ -f "/usr/include/rpc/rpc.h" ] && exit 0

[ -f "/usr/include/tirpc/rpc/rpc.h" ] && \
 echo "TIRPC=YES" >> "configure/CONFIG_SITE.Common.linux-x86_64"
