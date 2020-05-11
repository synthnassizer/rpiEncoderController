export PATH=$PATH:/opt/cross-pi-gcc/bin
export RPI_SYSROOT="${HOME}/workstuff/dev/rpi3/rpi3rootfs/"
export CC="arm-linux-gnueabihf-gcc --sysroot=${RPI_SYSROOT}"
export CXX="arm-linux-gnueabihf-g++ --sysroot=${RPI_SYSROOT}"
export CPP="arm-linux-gnueabihf-gcc -E --sysroot=${RPI_SYSROOT}"
export AS="arm-linux-gnueabihf-as "
export LD="arm-linux-gnueabihf-ld  --sysroot=${RPI_SYSROOT}"
export GDB=arm-linux-gnueabihf-gdb
export STRIP=arm-linux-gnueabihf-strip
export RANLIB=arm-linux-gnueabihf-ranlib
export OBJCOPY=arm-linux-gnueabihf-objcopy
export OBJDUMP=arm-linux-gnueabihf-objdump
export AR=arm-linux-gnueabihf-ar
export NM=arm-linux-gnueabihf-nm
export M4=m4
export TARGET_PREFIX="arm-linux-gnueabihf-"
export CONFIGURE_FLAGS="--target=arm-linux-gnueabihf --host=arm-linux-gnueabihf --build=x86_64-linux --sysroot=${RPI_SYSROOT}"
#export CFLAGS=" -pipe -g -feliminate-unused-debug-types "
export CFLAGS=" -pipe -g -feliminate-unused-debug-types -I${RPI_SYSROOT}/usr/include -L${RPI_SYSROOT}/usr/lib -I${RPI_SYSROOT}/usr/include/arm-linux-gnueabihf -L${RPI_SYSROOT}/usr/lib/arm-linux-gnueabihf" 
#export CXXFLAGS=" -pipe -g -feliminate-unused-debug-types -fpermissive "
export CXXFLAGS=" -pipe -g -feliminate-unused-debug-types -fpermissive -I${RPI_SYSROOT}/usr/include -L${RPI_SYSROOT}/usr/lib -I${RPI_SYSROOT}/usr/include/arm-linux-gnueabihf -L${RPI_SYSROOT}/usr/lib/arm-linux-gnueabihf"
#export CPPFLAGS=" -pipe -g -feliminate-unused-debug-types -fpermissive "
export CPPFLAGS=" -pipe -g -feliminate-unused-debug-types -fpermissive -I${RPI_SYSROOT}/usr/include -L${RPI_SYSROOT}/usr/lib -I${RPI_SYSROOT}/usr/include/arm-linux-gnueabihf -L${RPI_SYSROOT}/usr/lib/arm-linux-gnueabihf"
export LDFLAGS="-Wl,-O1 -Wl,--hash-style=gnu -Wl,--as-needed -Wl,-rpath-link=${RPI_SYSROOT}/usr/lib/arm-linux-gnueabihf"
export ARCH=arm
export RASPBIAN_ROOTFS=${RPI_SYSROOT}
export RASPBERRY_VERSION=3
