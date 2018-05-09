INCLUDE_DIR:=$(TOP_DIR)/include
SCRIPT_DIR:=$(TOP_DIR)/scripts
BUILD_DIR:=$(TOP_DIR)/build_dir
PACKAGE_DIR:=$(TOP_DIR)/packages
TARGET_DIR:=$(TOP_DIR)/target/rootfs
SHARED_DIR:=$(TOP_DIR)/shared
PKG_BUILD_DIR=$(TOP_DIR)/build_dir
PKG_INSTALL_DIR = $(BUILD_PATH)/pkg-install

CC:=gcc
CXX:=g++
MAKE=make
CP:=cp -fpR
LN:=ln -sf
XARGS:=xargs -r
CONFIGURE=configure
PRE_CONFIGURE= echo no preconfigre
MAKEFILE_NAME=Makefile

INSTALL_BIN:=install -m0755
INSTALL_DIR:=install -d -m0755
INSTALL_DATA:=install -m0644
INSTALL_CONF:=install -m0600

#needs to be override if the value is null by default and we do need some values
BUILD_PATH=
SRC_PATH=
CONFIGURE_PATH=
MAKE_PATH=

DEF_CONFIGURE_VARS = --prefix=$(PKG_INSTALL_DIR)
CONFIGRE_VARS=
MAKE_VARS =
MAKE_INSTALL_FLAGS =

MAKE_FLAGS = $(TARGET_CONFIGURE_OPTS)
PKG_JOBS="-j 4"

CFLAG += -I$(SHARED_DIR)/include -I$(SHARED_DIR)/usr/include -I$(SHARED_DIR)/usr/local/include
LDFLAG += -L$(SHARED_DIR)/lib -L$(SHARED_DIR)/usr/lib -L$(SHARED_DIR)/usr/local/lib

#EXT_CFLAGS += -I$(SHARED_DIR)/include -I$(SHARED_DIR)/usr/include -I$(SHARED_DIR)/usr/local/include
#EXT_LDFLAGS += -L$(SHARED_DIR)/lib -L$(SHARED_DIR)/usr/lib -L$(SHARED_DIR)/usr/local/lib
EXT_CFLAGS += -I$(PUBLIB_DIR)/include
EXT_LDFLAGS += -L$(PUBLIB_DIR)/lib
	
export EXT_CFLAGS EXT_LDFLAGS
