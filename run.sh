#!/usr/bin/bash
DIR="$( cd -P "$( dirname "$0" )" && pwd )"
pushd ${DIR}
ROOT_DIR=${PWD}
popd

TODAY=`date +%Y%m%d`
UNAME=`uname`
BUILD_TYPE="Debug"
ARCHIVE=${BUILD_DIR}/archive
BUILD_DIR=${ROOT_DIR}/build_mt 

TEST_FILE=/home/sander/Downloads/aligned.ifc

echo "##############################################################################"
echo "##  DIR            :  ${DIR}"
echo "##  ROOT DIR       :  ${ROOT_DIR}"
echo "##  BUILD DIR      :  ${BUILD_DIR}"
echo "##  CMAKE settings :  ${CMAKE_SETTINGS}"

if [ ! -d $BUILD_DIR ]; then
  mkdir -p $BUILD_DIR;
fi
pushd $BUILD_DIR
time ./IfcConvert -y ${TEST_FILE} test.obj
popd
