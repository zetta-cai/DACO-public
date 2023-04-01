#!/usr/bin/env bash

echo "Download lib/boost_1_81_0.tar.gz..."
wget https://boostorg.jfrog.io/artifactory/main/release/1.81.0/source/boost_1_81_0.tar.gz
mv boost_1_81_0.tar.gz lib/
echo "Decompress lib/boost_1_81_0.tar.gz..."
cd lib
tar -xzvf boost_1_81_0.tar.gz
echo "Install lib/boost from source..."
cd boost_1_81_0
./bootstrap.sh --with-libraries=log,thread,system,filesystem,program_options,test --prefix=./boost
sudo ./b2 install
# Uncomment this command if your set prefix as /usr/local which needs to update Linux dynamic linker
#sudo ldconfig
cd ..
#echo "Remove lib/boost_1_81_0 and lib/boost_1_81_0.tar.gz..."
#rm boost_1_81_0.tar.gz
#rm -r boost_1_81_0
cd ..