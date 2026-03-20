#!/bin/sh

UPSTREAM_VERSION=$(dpkg-parsechangelog | sed -e '/^Version/!d' -e 's/^Version: //g' -e 's/-.*//g')
UPSTREAM_REVISION=$(echo ${UPSTREAM_VERSION} | sed -e 's/.*svn/-r/')
UPSTREAM_SVN=https://ssl.bulix.org/svn/lcd4linux/trunk/
UPSTREAM_CHECKOUT=lcd4linux-${UPSTREAM_VERSION}
OLDDIR=${PWD}
GOS_DIR=${OLDDIR}/get-orig-source

if [ -z ${UPSTREAM_VERSION} ]; then
	echo 'Please run this script from the sources root directory.'
	exit 1
fi

rm -rf ${GOS_DIR}
mkdir ${GOS_DIR} && cd ${GOS_DIR}
svn export ${UPSTREAM_REVISION} ${UPSTREAM_SVN} ${UPSTREAM_CHECKOUT}
tar -zcf ../../lcd4linux_${UPSTREAM_VERSION}.orig.tar.gz ${UPSTREAM_CHECKOUT}
rm -rf ${GOS_DIR}
