#! /bin/bash
# sudo apt-get update
# sudo apt-get install build-essential

NM_VERSION=1.1.5
if [ ! -d nanomsg-${NM_VERSION} ]; then
  wget --no-check-certificate https://github.com/nanomsg/nanomsg/archive/${NM_VERSION}.tar.gz
  tar xzf ${NM_VERSION}.tar.gz
fi
cd nanomsg-${NM_VERSION}
mkdir build
cd build
cmake ..
cmake --build .
ctest .
sudo cmake --build . --target install
sudo ldconfig
