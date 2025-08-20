#!/usr/bin/env bash

cd $(dirname $0)

SELF_DIR=$(pwd)
ROOT_DIR=$SELF_DIR

BUILD_ARCHIVE="llama-cpp"

[ -d build ] && rm -rf build

[ ! -d build ] && mkdir build

cmake -B build -DGGML_RPC=ON -DLLAMA_CURL=ON

cmake --build build --config Release -j $(nproc) --target rpc-server --target llama-server


NR_CPUS=$(nproc 2>/dev/null)

CUR_VERSION="1.0.0"

CPU_ARCH="all"
TEST_BIN="/usr/bin/ls"
is_amd64=$(file ${TEST_BIN} 2>/dev/null | grep -i -E "x86-64|x86_64|amd64")
is_arm64=$(file ${TEST_BIN} 2>/dev/null | grep -i -E "aarch64|arm64")
if [ -n "$is_amd64" ]; then
    CPU_ARCH="amd64"
elif [ -n "$is_arm64" ]; then
    CPU_ARCH="arm64"
else
    pr_err "unknown cpu architecture"
    exit 1
fi

[ -e ${ROOT_DIR}/debbuild ] && rm -rf ${ROOT_DIR}/debbuild
mkdir -p ${ROOT_DIR}/debbuild/DEBIAN

cat > ${ROOT_DIR}/debbuild/DEBIAN/control << EOF
Package: ${BUILD_ARCHIVE}
Version: ${CUR_VERSION}
Section: utils
Installed-Size: 0
Priority: optional
Architecture: ${CPU_ARCH}
Maintainer: Laxcus <laxcus@163.com>
Description: LLM inference in C/C++
EOF

cat > ${ROOT_DIR}/debbuild/DEBIAN/postinst << EOF
#!/bin/bash

exit 0
EOF
chmod 755 ${ROOT_DIR}/debbuild/DEBIAN/postinst

cat > ${ROOT_DIR}/debbuild/DEBIAN/postrm << EOF
#!/bin/bash
case "\$1" in
    remove)
        exit 0
        ;;

    purge)
        exit 0
        ;;

    *)
        ;;
esac

exit 0
EOF
chmod 755 ${ROOT_DIR}/debbuild/DEBIAN/postrm

#
# dpkg-deb -c xxx.deb
# dpkg-deb -I xxx.deb
#

mkdir -p debbuild/opt/lxllama/llama/bin/
cp -rf build/bin/* debbuild/opt/lxllama/llama/bin/

INSTALLED_SIZE=$(du -sk --exclude=DEBIAN debbuild | awk '{print $1}')
if grep -q "Installed-Size" debbuild/DEBIAN/control; then
    sed -i "s/Installed-Size: .*/Installed-Size: $INSTALLED_SIZE/" debbuild/DEBIAN/control
fi

rm -f *.deb
dpkg-deb --build debbuild ${BUILD_ARCHIVE}_${CUR_VERSION}_${CPU_ARCH}.deb

exit 0
