#!/usr/bin/env bash

DEVICE_ADDRESS="192.168.1.64"
PTX_PATH="../ptxproj"
PROJECT_NAME="winner-power-reader"
PKG_NAME="${PROJECT_NAME}_0.1_armhf.ipk"
PKG_PATH="platform-wago-pfcXXX/packages/${PKG_NAME}"

cp -fr src/* "${PTX_PATH}/src/${PROJECT_NAME}"
cd $PTX_PATH
ptxdist clean $PROJECT_NAME
set -o pipefail
ptxdist targetinstall $PROJECT_NAME | grep -v dlsym

retval=$?
if [[ $retval -ne 0 ]]
then
	echo "Failed to build the project" >&2
	exit $retval
fi

scp $PKG_PATH "root@${DEVICE_ADDRESS}:~/"
ssh "root@${DEVICE_ADDRESS}" "opkg install --force-reinstall ${PKG_NAME}"
#ssh "root@${DEVICE_ADDRESS}" powerreader
ssh "root@${DEVICE_ADDRESS}"
