./configure  CC="/opt/gcw0-toolchain/usr/bin/mipsel-gcw0-linux-uclibc-gcc" CFLAGS="-D_POSIX_C_SOURCE" CFLAGS_BUILD="-D_POSIX_C_SOURCE"

make all

cp objs/release/opendune opk
cp /opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/lib/libpulse.so.0 opk
cp /opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/lib/libjson-c.so.2 opk
cp /opt/gcw0-toolchain/usr/mipsel-gcw0-linux-uclibc/sysroot/usr/lib/pulseaudio/libpulsecommon-5.0.so opk
mksquashfs opk opendune.opk

