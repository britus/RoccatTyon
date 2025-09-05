#!/bin/bash -e

function command_help
{
	echo "ERROR: $1!"
	echo "Usage: $0 <SOURCE_DIR> <OUTPUT_DIR>"
	exit 1
}

SOURCE_DIR="$1"
OUTPUT_DIR="$2"

if [ -z ${SOURCE_DIR} ] ; then
	command_help "HIDAPI: Source directory is missing!"
	exit 1
fi

if [ -z ${OUTPUT_DIR} ] ; then
	command_help "HIDAPI: Output directory is missing!"
	exit 1
fi

if [ "x${SOURCE_DIR}" == "x-h" -o "x${SOURCE_DIR}" == "x--help" ] ; then
	command_help "Help ------------------------ "
	exit 1
fi

BUILD_DIR="${OUTPUT_DIR}/hidapi"

echo "-------------------------------------------------------------------"
echo "hidapi-BUILD-SRC : ${SOURCE_DIR}"
echo "hidapi-BUILD-OUT : ${BUILD_DIR}"
echo "hidapi-OUTPUT_DIR: ${OUTPUT_DIR}"
echo "Current directory: `pwd`"
echo "-------------------------------------------------------------------"

if [ ! -x ${SOURCE_DIR}/bootstrap ] ; then
		echo "Invalid source directory: ${SOURCE_DIR}"
		echo "Missing executable 'bootstrap' script."
		exit 1
fi

#if [ ! -x ${SOURCE_DIR}/configure ] ; then
#		echo "Invalid source directory: ${SOURCE_DIR}"
#		echo "Missing executable 'configure' script."
#		exit 1
#fi

if [ -f ${OUTPUT_DIR}/.hidapi ] ; then
	echo "hidapi: Build already complete."
	exit 0
fi

# force qmake build by removing .pretarget
# that will be recreated at end of build
rm -f ${OUTPUT_DIR}/.hidapi

mkdir -p ${BUILD_DIR}   || exit 1
mkdir -p ${OUTPUT_DIR}  || exit 1

echo "HIDAPI: Fetch and update source base..."
cd ${SOURCE_DIR} && git fetch --all && git pull

echo "HIDAPI: Copy to build directory..."
cd -
cp -R ${SOURCE_DIR}/* ${BUILD_DIR}/  || exit 1

# Anti perl localization messages
unset LC_MESSAGES
export LANG=C

# Mac OSX dependencies
PLATFORM=`uname -s`

if [ "x${PLATFORM}" == "xDarwin" ] ; then
	HOMEBREW_BIN="/opt/homebrew/bin"
	PATH=.:${HOMEBREW_BIN}:${PATH}

	if [ -x /usr/local/bin/brew ] ; then
		${HOMEBREW_BIN}/brew install automake autoconf libtool
	fi
fi

#printenv 1>&2 | sort -u

echo "HIDAPI: Enter build dir: ${BUILD_DIR}"
cd ${BUILD_DIR}

if [ ! -r ${BUILD_DIR}/.done1 ] ; then
	echo "HIDAPI: Prepare user space library..."
	if [ -f ${BUILD_DIR}/bootstrap ] ; then
		chmod 755 ./bootstrap && ./bootstrap 1>/dev/null
		if [ $? -ne 0 ] ; then
			echo "HIDAPI: bootstrap failed."
			exit 1
		fi
	else
		if [ ! -f NEWS ] ; then
			touch NEWS
		fi
		# The common ChangeLog is maintained on the top level of trunk
		if [ ! -f ChangeLog ] ; then
			touch ChangeLog
		fi
		aclocal -I m4 1>/dev/null && \
		libtoolize --copy 1>/dev/null && \
		autoheader 1>/dev/null && \
		autoconf 1>/dev/null && \
		automake -a -c 1>/dev/null
		if [ $? -ne 0 ] ; then
			echo "HIDAPI: bootstrap failed."
			exit 1
		fi
	fi
	touch ${BUILD_DIR}/.done1
fi

if [ ! -r ${BUILD_DIR}/.done2 ] ; then
	echo "HIDAPI: Configure library build..."
	cd ${BUILD_DIR} && \
		chmod 755 ./configure && \
		./configure --prefix=/usr/local 1>/dev/null
	if [ $? -ne 0 ] ; then
			echo "HIDAPI: configure failed."
			exit 1
	fi
	touch ${BUILD_DIR}/.done2
fi

XLDCONFIG="echo 'OK'"
if [ -f /sbin/ldconfig ] ; then
	XLDCONFIG="/sbin/ldconfig"
fi

if [ ! -r ${BUILD_DIR}/.done3 ] ; then
	echo "HIDAPI: Build and install library..."
	cd ${BUILD_DIR} && \
		make clean 1>/dev/null && \
		make 1>/dev/null && \
		sudo make install 1>/dev/null && \
		${XLDCONFIG}
	if [ $? -ne 0 ] ; then
		echo "HIDAPI: make and install failed."
		rm -f ${BUILD_DIR}/.done2
		exit 1
	fi
	touch ${BUILD_DIR}/.done3
fi

# completed
echo "HIDAPI: Build successfully completed."
echo "1" > ${OUTPUT_DIR}/.hidapi
exit 0
