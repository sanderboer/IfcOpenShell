#!/usr/bin/bash
DIR="$( cd -P "$( dirname "$0" )" && pwd )"
pushd ${DIR}
ROOT_DIR=${PWD}
popd

TODAY=`date +%Y%m%d`
UNAME=`uname`
BUILD_TYPE="Debug"
ARCHIVE=${BUILD_DIR}/archive
BUILD_DIR=${ROOT_DIR}/build_master

SYSTEM_OCCT_PATH="/usr"
OCC_INCLUDE_DIR="${SYSTEM_OCCT_PATH}/include/opencascade"
OCC_LIBRARY_DIR="${SYSTEM_OCCT_PATH}/lib"

GENERATOR="Unix Makefiles"

NO_WARNINGS="-Wno-overloaded-virtual -Wno-ignored-qualifiers"
CMAKE_SETTINGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=ON "
CMAKE_SETTINGS+="-DUSE_IFC4=OFF "
CMAKE_SETTINGS+="-DCOLLADA_SUPPORT=OFF "
CMAKE_SETTINGS+="-DBUILD_IFCPYTHON=OFF "
CMAKE_SETTINGS+="-DOCC_INCLUDE_DIR=${OCC_INCLUDE_DIR} "
CMAKE_SETTINGS+="-DOCC_LIBRARY_DIR=${OCC_LIBRARY_DIR} "
CMAKE_SETTINGS+="-DUNICODE_SUPPORT=OFF "
CMAKE_SETTINGS+="-DBUILD_EXAMPLES=OFF "
CMAKE_SETTINGS+="-DCMAKE_BUILD_TYPE=${BUILD_TYPE} "
CMAKE_SETTINGS+="-DCMAKE_INSTALL_PREFIX=${ARCHIVE} "

# CMAKE_SETTINGS+="-DCMAKE_C_COMPILER=clang "
# CMAKE_SETTINGS+="-DCMAKE_CXX_COMPILER=clang++ "
# CMAKE_SETTINGS+="-DCMAKE_C_FLAGS_DEBUG=-O0 -ggdb ${NO_WARNINGS} "
# CMAKE_SETTINGS+="-DCMAKE_CXX_FLAGS_DEBUG=-O0 -ggdb -Wall ${NO_WARNINGS} "
# CMAKE_SETTINGS+="-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wno-ignored-qualifiers "


echo "##############################################################################"
echo "##  DIR            :  ${DIR}"
echo "##  ROOT DIR       :  ${ROOT_DIR}"
echo "##  BUILD DIR      :  ${BUILD_DIR}"
echo "##  OCCT path      :  ${OCCT_PATH}"
echo "##  CMAKE settings :  ${CMAKE_SETTINGS}"

if [ ! -d $BUILD_DIR ]; then
  mkdir -p $BUILD_DIR;
fi
cd $BUILD_DIR

cmake -G"${GENERATOR}" ${CMAKE_SETTINGS} $ROOT_DIR/cmake/
