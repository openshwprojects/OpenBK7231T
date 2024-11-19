#!/bin/sh
APP_BIN_NAME=$1
APP_VERSION=$2
TARGET_PLATFORM=$3
BUILD_NUMBER=$4
USER_CMD=$5
BUILD_MODE=$6

echo "From build.sh, variables are:"
echo APP_BIN_NAME=$APP_BIN_NAME
echo APP_VERSION=$APP_VERSION
echo TARGET_PLATFORM=$TARGET_PLATFORM
echo USER_CMD=$USER_CMD
echo BUILD_NUMBER=$BUILD_NUMBER

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`

echo "Start Compile"
set -e

SYSTEM=`uname -s`
echo "system:"$SYSTEM
if [ $SYSTEM = "Linux" ]; then
	TOOL_DIR=package_tool/linux
	OTAFIX=${TOOL_DIR}/otafix
	ENCRYPT=${TOOL_DIR}/encrypt
	BEKEN_PACK=${TOOL_DIR}/beken_packager
	RT_OTA_PACK_TOOL=${TOOL_DIR}/rt_ota_packaging_tool_cli
	TY_PACKAGE=${TOOL_DIR}/package
else
	TOOL_DIR=package_tool/windows
	OTAFIX=${TOOL_DIR}/otafix.exe
	ENCRYPT=${TOOL_DIR}/encrypt.exe
	BEKEN_PACK=${TOOL_DIR}/beken_packager.exe
	RT_OTA_PACK_TOOL=${TOOL_DIR}/rt_ota_packaging_tool_cli.exe
	TY_PACKAGE=${TOOL_DIR}/package.exe
fi

APP_PATH=../../../apps

echo removing .o files from our folders, and OUR .o and .d files from Debug

for i in `find ${APP_PATH}/$APP_BIN_NAME/src -type d`
do
	# build our stuff all the time - there is not much and reminds us about all the warnings
    rm -rf $i/*.o

	for j in `find $i/*.c`
	do
		FILE=${j##*/}
		FILE=${FILE%.*}
    	rm -rf ./Debug/obj/${FILE}.d
    	rm -rf ./Debug/obj/${FILE}.o
		# echo removing ./Debug/obj/${FILE}.d
		# echo removing ./Debug/obj/${FILE}.o
	done
done

for i in `find ../tuya_common/src -type d`
do
	# don't build tuya every time
    echo not rm -rf $i/*.o
done

for i in `find ../../../components -type d`
do
	# don't build components every time
    echo not rm -rf $i/*.o
done

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
else
	make APP_BIN_NAME=$APP_BIN_NAME USER_SW_VER=$USER_SW_VER APP_VERSION=$APP_VERSION BUILD_NUMBER=$BUILD_NUMBER clean -C ./
fi

make APP_BIN_NAME=$APP_BIN_NAME USER_SW_VER=$USER_SW_VER APP_VERSION=$APP_VERSION $USER_CMD BUILD_NUMBER=$BUILD_NUMBER -j -C ./

echo "Start Combined"
cp ${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.bin tools/generate/
echo "Combined will do cd"
cd tools/generate
echo "Combined will do OTAFIX"
./${OTAFIX} ${APP_BIN_NAME}_${APP_VERSION}.bin
echo "Combined will do ENCRYPT"
./${ENCRYPT} ${APP_BIN_NAME}_${APP_VERSION}.bin 510fb093 a3cbeadc 5993a17e c7adeb03 10000
echo "Combined will do mpytools.py"
python mpytools.py ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
echo "Combined will do BEKEN_PACK"
./${BEKEN_PACK} config.json

echo "End Combined"
cp all_1.00.bin ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin
rm all_1.00.bin

cp ${APP_BIN_NAME}_${APP_VERSION}_enc_uart_1.00.bin ${APP_BIN_NAME}_UA_${APP_VERSION}.bin
rm ${APP_BIN_NAME}_${APP_VERSION}_enc_uart_1.00.bin

#generate ota file
echo "generate ota file"
./${RT_OTA_PACK_TOOL} -f ${APP_BIN_NAME}_${APP_VERSION}.bin -v $CURRENT_TIME -o ${APP_BIN_NAME}_${APP_VERSION}.rbl -p app -c gzip -s aes -k 0123456789ABCDEF0123456789ABCDEF -i 0123456789ABCDEF
./${TY_PACKAGE} ${APP_BIN_NAME}_${APP_VERSION}.rbl ${APP_BIN_NAME}_UG_${APP_VERSION}.bin $APP_VERSION 
echo rm ${APP_BIN_NAME}_${APP_VERSION}.rbl
rm ${APP_BIN_NAME}_${APP_VERSION}.bin
rm ${APP_BIN_NAME}_${APP_VERSION}.cpr
rm ${APP_BIN_NAME}_${APP_VERSION}.out
rm ${APP_BIN_NAME}_${APP_VERSION}_enc.bin

echo "ug_file size:"
ls -l ${APP_BIN_NAME}_UG_${APP_VERSION}.bin | awk '{print $5}'
if [ `ls -l ${APP_BIN_NAME}_UG_${APP_VERSION}.bin | awk '{print $5}'` -gt 679936 ];then
	echo "**********************${APP_BIN_NAME}_$APP_VERSION.bin***************"
	echo "************************** too large ********************************"
	rm ${APP_BIN_NAME}_UG_${APP_VERSION}.bin
	rm ${APP_BIN_NAME}_UA_${APP_VERSION}.bin
	rm ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin
	exit 1
fi

echo "$(pwd)"
cp ${APP_BIN_NAME}_${APP_VERSION}.rbl ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}_build${BUILD_NUMBER}.rbl
cp ${APP_BIN_NAME}_UG_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UG_${APP_VERSION}_build${BUILD_NUMBER}.bin
cp ${APP_BIN_NAME}_UA_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UA_${APP_VERSION}_build${BUILD_NUMBER}.bin
cp ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_QIO_${APP_VERSION}_build${BUILD_NUMBER}.bin

echo "*************************************************************************"
echo "*************************************************************************"
echo "*************************************************************************"
echo "*********************${APP_BIN_NAME}_${APP_VERSION}_build${BUILD_NUMBER}.bin********************"
echo "*************************************************************************"
echo "**********************COMPILE SUCCESS************************************"
echo "*************************************************************************"

FW_NAME=$APP_NAME
if [ -n $CI_IDENTIFIER ]; then
        FW_NAME=$CI_IDENTIFIER
fi

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
	exit
else
	mkdir -p ${CI_PACKAGE_PATH}

   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UG_${APP_VERSION}_build${BUILD_NUMBER}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_UG_"$APP_VERSION.bin
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UA_${APP_VERSION}_build${BUILD_NUMBER}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_UA_"$APP_VERSION.bin
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_QIO_${APP_VERSION}_build${BUILD_NUMBER}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_QIO_"$APP_VERSION.bin
fi
