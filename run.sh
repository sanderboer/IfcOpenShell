#!/usr/bin/bash
DIR="$( cd -P "$( dirname "$0" )" && pwd )"
pushd ${DIR}
ROOT_DIR=${PWD}
popd

TODAY=`date +%Y%m%d`
UNAME=`uname`
BUILD_TYPE="Debug"
BUILD_DIR=${ROOT_DIR}/build_master 
BUILD_DIR_MT=${ROOT_DIR}/build_mt 

# TEST_FILE=/home/sander/Downloads/aligned.ifc
TEST_FILE=${ROOT_DIR}/test-bool.ifc

echo "##############################################################################"
echo "##  DIR            :  ${DIR}"
echo "##  ROOT DIR       :  ${ROOT_DIR}"

pushd $BUILD_DIR
#time ./IfcConvert -y ${TEST_FILE} test.obj
popd

pushd $BUILD_DIR_MT
time ./IfcConvert -y ${TEST_FILE} test.obj
popd
