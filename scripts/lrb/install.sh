#!/bin/bash
set -e

# Siyuan: NO need for LRB
## assume current path is under webcachesim
#sudo apt-get update
#sudo apt install -y git cmake build-essential libboost-all-dev python3-pip parallel libprocps-dev software-properties-common
## install openjdk 1.8. This is for simulating Adaptive-TinyLFU. The steps has to be one by one
#sudo add-apt-repository ppa:openjdk-r/ppa -y
#sudo apt-get update
#sudo apt-get install -y openjdk-8-jdk
#java -version  # output should be 1.8

# Siyuan: --debug-find can be used to dump debug info of finding header/lib/module files

# Siyuan: lib/lrb by default
lrb_rootpath=$(pwd)

# Siyuan: use the same version of libboost (lib/boost_1_81_0/install by default)
libboost_prefix_path=${lrb_rootpath}/../boost_1_81_0/install

cd ./lib

# install LightGBM
cd ./LightGBM/build
# Siyuan: ONLY report to env variable, CMAKE_PREFIX_PATH can affect search paths in cmake
CMAKE_PREFIX_PATH=${libboost_prefix_path}
export CMAKE_PREFIX_PATH
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${lrb_rootpath}/install/lightgbm ..
make -j
sudo make install
cd ../..

# dependency for mongo c driver
sudo apt-get install -y cmake libssl-dev libsasl2-dev libprocps-dev

# installing mongo c
cd ./mongo-c-driver/cmake-build
# Siyuan: ONLY report to env variable, CMAKE_PREFIX_PATH can affect search paths in cmake
CMAKE_PREFIX_PATH=${libboost_prefix_path}
export CMAKE_PREFIX_PATH
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_INSTALL_PREFIX=${lrb_rootpath}/install/mongocdriver ..
make -j
sudo make install
cd ../..

# installing mongo-cxx
cd ./mongo-cxx-driver/build
# Siyuan: ONLY report to env variable, CMAKE_PREFIX_PATH can affect search paths in cmake
CMAKE_PREFIX_PATH=${libboost_prefix_path}:${lrb_rootpath}/install/mongocdriver # Siyuan: mongo-cxx-driver relies on lib/lrb/install/mongocdriver/lib/cmake/libbson-1.0/libbson-1.0-config.cmake to find libbson installed by mongo-c-driver
export CMAKE_PREFIX_PATH
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${lrb_rootpath}/install/mongocxxdriver ..
sudo make VERBOSE=1 EP_mnmlstc_core >/home/sysheng/projects/covered-private/tmp.out 2>&1 # TMPDEBG240109
make -j
sudo make install
cd ../..

# installing libbf
cd ./libbf/build
# Siyuan: ONLY report to env variable, CMAKE_PREFIX_PATH can affect search paths in cmake
CMAKE_PREFIX_PATH=${libboost_prefix_path}
export CMAKE_PREFIX_PATH
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${lrb_rootpath}/install/libbf ..
make -j
sudo make install
cd ../..
cd ..

# building webcachesim, install the library with api
cd ./build
CMAKE_PREFIX_PATH=${libboost_prefix_path}:${lrb_rootpath}/install/mongocxxdriver:${lrb_rootpath}/install/lightgbm:${lrb_rootpath}/install/libbf
export CMAKE_PREFIX_PATH
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${lrb_rootpath}/install/webcachesim ..
make -j
sudo make install
sudo ldconfig
cd ../

# Siyuan: clear CMAKE_PREFIX_PATH
CMAKE_PREFIX_PATH=""
export CMAKE_PREFIX_PATH