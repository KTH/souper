from ubuntu:20.04

run set -x; \
        echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections \
        && apt-get update -y -qq \
        && apt-get dist-upgrade -y -qq \
        && apt-get autoremove -y -qq \
        && apt-get remove -y -qq clang llvm llvm-runtime \
	&& apt-get install libgmp10 \
	&& echo 'ca-certificates valgrind libc6-dev libgmp-dev cmake patch ninja-build make autoconf automake libtool golang-go python subversion re2c git clang' > /usr/src/build-deps \
	&& apt-get install -y $(cat /usr/src/build-deps) --no-install-recommends \
	&& git clone https://github.com/antirez/redis /usr/src/redis

run export CC=clang CXX=clang++ \
        && cd /usr/src/redis \
	&& git checkout 5.0.3 \
	&& make -j10 \
	&& make install

run export GOPATH=/usr/src/go \
	&& go get github.com/gomodule/redigo/redis

add build_deps.sh /usr/src/souper/build_deps.sh
add clone_and_test.sh /usr/src/souper/clone_and_test.sh

run export CC=clang CXX=clang++ \
	&& cd /usr/src/souper \
#	&& ./build_deps.sh Debug \
#       && rm -rf third_party/llvm/Debug-build \
	&& bash ./build_deps.sh Release
	
run	rm -rf third_party/llvm/Release-build \
	&& rm -rf third_party/hiredis/install/lib/libhiredis.so*


run apt-get install -y llvm 


add CMakeLists.txt /usr/src/souper/CMakeLists.txt
add docs /usr/src/souper/docs
add include /usr/src/souper/include
add lib /usr/src/souper/lib
add runtime /usr/src/souper/runtime
add test /usr/src/souper/test
add tools /usr/src/souper/tools
add utils /usr/src/souper/utils
add unittests /usr/src/souper/unittests

run export GOPATH=/usr/src/go \
        && export LD_LIBRARY_PATH=/usr/src/souper/third_party/z3-install/lib:$LD_LIBRARY_PATH \
	&& mkdir -p /usr/src/souper-build \
	&& cd /usr/src/souper-build \
	&& cmake -G Ninja ../souper \
	&& ninja 
run ls /usr/src/souper-build
