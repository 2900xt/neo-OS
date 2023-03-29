#compile and build the x86_64-elf-gcc cross compiler
#
#requisites:
# 1.libmpc-dev
# 2.libmpfr-dev
# 3.libgmp-dev

sudo apt update

sudo apt install build-essential

sudo apt install bison

sudo apt install flex

sudo apt install libgmp3-dev

sudo apt install libmpc-dev

sudo apt install libmpfr-dev

sudo apt install texinfo

sudo apt install nasm

sudo apt install qemu-desktop

export PREFIX="/usr/local/x86_64elfgcc"

export TARGET=x86_64-elf

export PATH="$PREFIX/bin:$PATH"

mkdir /tmp/src

cd /tmp/src

curl -O http://ftp.gnu.org/gnu/binutils/binutils-2.35.1.tar.gz

tar xf binutils-2.35.1.tar.gz

mkdir binutils-build

cd binutils-build

../binutils-2.35.1/configure --target=$TARGET --enable-interwork --enable-multilib --disable-nls --disable-werror --prefix=$PREFIX 2>&1 | tee configure.log

sudo make all install 2>&1 | tee make.log

cd /tmp/src

curl -O https://ftp.gnu.org/gnu/gcc/gcc-10.2.0/gcc-10.2.0.tar.gz

tar xf gcc-10.2.0.tar.gz

mkdir gcc-build

cd gcc-build

../gcc-10.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --disable-libssp --enable-languages=c++ --without-headers

sudo make all-gcc

sudo make all-target-libgcc

sudo make install-gcc

sudo make install-target-libgcc
