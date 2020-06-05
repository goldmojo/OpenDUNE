# RG350 configuration switches
# Without pulseaudio
# With SDL2 instead of SDL

# Normal mode
./configure --with-sdl2 --with-asound --without-oss --without-pulse --without-munt CC="/opt/gcw0-toolchain/usr/bin/mipsel-gcw0-linux-uclibc-gcc" CFLAGS="-D_POSIX_C_SOURCE -D__GCW0__" CFLAGS_BUILD="-D_POSIX_C_SOURCE -D__GCW0__"

# Debug mode
#./configure --with-sdl2 --with-asound --without-oss --without-pulse --without-munt CC="/opt/gcw0-toolchain/usr/bin/mipsel-gcw0-linux-uclibc-gcc" CFLAGS="-D_POSIX_C_SOURCE -D__GCW0__ -D_DEBUG" CFLAGS_BUILD="-D_POSIX_C_SOURCE -D__GCW0__ -D_DEBUG"

make all
cp objs/release/opendune opk
rm -f opendune.opk
mksquashfs opk opendune.opk

lftp -e "cd /apps;put opendune.opk;quit" 10.1.1.2
