./configure --with-pulse="0" CC="/opt/gcw0-toolchain/usr/bin/mipsel-gcw0-linux-uclibc-gcc" CFLAGS="-D_POSIX_C_SOURCE" CFLAGS_BUILD="-D_POSIX_C_SOURCE"

make all
cp objs/release/opendune opk
rm -f opendune.opk
mksquashfs opk opendune.opk
