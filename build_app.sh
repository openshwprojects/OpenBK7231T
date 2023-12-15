#!/bin/sh
fatal() {
    echo "\033[0;31merror: $1\033[0m"
    echo
    echo "Usage: $0 app_path app_name [app_version [user_cmd]]"
    echo "see https://github.com/openshwprojects/OpenBK7231T_App/ for full info"
    exit 1
}



APP_PATH=$1
APP_NAME=$2
APP_VERSION=$3
USER_CMD=$4

[ -z $APP_PATH ] && fatal "no app path!"
[ -z $APP_NAME ] && fatal "no app name!"

[ "$APP_VERSION" = "git" ] && APP_VERSION="`(head -c8 $APP_PATH/.git/refs/heads/main)`"
[ -z "$APP_VERSION" ] && [ -z $USER_CMD ] && APP_VERSION="`(head -c8 $APP_PATH/.git/refs/heads/main)`"

if [ -z "${APP_VERSION}" ]; then 
	echo "App version not specified (or git command failed on Cygwin), using 1.0.0"
    APP_VERSION='1.0.0'
fi

echo APP_PATH=$APP_PATH
echo APP_NAME=$APP_NAME
echo APP_VERSION=$APP_VERSION
echo USER_CMD=$USER_CMD


[ -z $APP_VERSION ] && fatal "no version!"


DEBUG_FLAG=`echo $APP_VERSION | sed -n 's,^[0-9]\+\.\([0-9]\+\)\.[0-9]\+\.*$,\1,p'`
if [ $((DEBUG_FLAG%2))=0 ]; then
    export APP_DEBUG=1
fi


cd `dirname $0`

TARGET_PLATFORM=bk7231t
TARGET_PLATFORM_REPO=https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/smart/embed/pruduct/bk7231t_1.0.22.zip
TARGET_PLATFORM_VERSION=1.0.22-beta.1
ROOT_DIR=$(pwd)

# 下载编译环境
if [ ! -d platforms/$TARGET_PLATFORM ]; then
    if [ -n "$TARGET_PLATFORM_REPO" ]; then
        # download toolchain
        cd platforms
        wget $TARGET_PLATFORM_REPO 
        unzip -o ${TARGET_PLATFORM}_${TARGET_PLATFORM_VERSION}.zip
        mv ${TARGET_PLATFORM}_${TARGET_PLATFORM_VERSION}_temp ${TARGET_PLATFORM}
        rm ${TARGET_PLATFORM}_${TARGET_PLATFORM_VERSION}.zip
        cd -
    fi
fi

# 判断当前编译环境是否OK
PLATFORM_BUILD_PATH_FILE=${ROOT_DIR}/platforms/$TARGET_PLATFORM/toolchain/build_path
if [ -e $PLATFORM_BUILD_PATH_FILE ]; then
    . $PLATFORM_BUILD_PATH_FILE
    if [ -n "$TUYA_SDK_TOOLCHAIN_ZIP" ];then
        if [ ! -f ${ROOT_DIR}/platforms/${TARGET_PLATFORM}/toolchain/${TUYA_SDK_BUILD_PATH}gcc ]; then
            echo "unzip file $TUYA_SDK_TOOLCHAIN_ZIP"
            tar -xf ${ROOT_DIR}/platforms/$TARGET_PLATFORM/toolchain/$TUYA_SDK_TOOLCHAIN_ZIP -C ${ROOT_DIR}/platforms/$TARGET_PLATFORM/toolchain/
            echo "unzip finish"
        fi
    fi
else
    echo "$PLATFORM_BUILD_PATH_FILE not found in platform[$TARGET_PLATFORM]!"
fi

if [ -z "$TUYA_SDK_BUILD_PATH" ]; then
    COMPILE_PREX=
else
    COMPILE_PREX=${ROOT_DIR}/platforms/$TARGET_PLATFORM/toolchain/$TUYA_SDK_BUILD_PATH
fi

cd $APP_PATH
if [ -f build.sh ]; then
    sh ./build.sh $APP_NAME $APP_VERSION $TARGET_PLATFORM $USER_CMD
elif [ -f Makefile -o -f makefile ]; then
    export COMPILE_PREX TARGET_PLATFORM
    make APP_BIN_NAME=$APP_NAME APP_NAME=$APP_NAME APP_VERSION=$APP_VERSION USER_SW_VER=$APP_VERSION USER_CMD=$USER_CMD all
elif [ -f ${ROOT_DIR}/platforms/$TARGET_PLATFORM/toolchain/$TUYA_APPS_BUILD_PATH/$TUYA_APPS_BUILD_CMD ]; then
    cd ${ROOT_DIR}/platforms/$TARGET_PLATFORM/toolchain/$TUYA_APPS_BUILD_PATH
    sh $TUYA_APPS_BUILD_CMD $APP_NAME $APP_VERSION $TARGET_PLATFORM $USER_CMD
else
    echo "No Build Command!"
    exit 1
fi

