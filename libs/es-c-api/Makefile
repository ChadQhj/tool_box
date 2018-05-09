include $(TOP_DIR)/rules.mk
include $(INCLUDE_DIR)/default.mk

SRC_DIR=src
MAKE_PATH=src
CONFIGURE_PATH=src
BUILD_PATH=$(BUILD_DIR)/es-c-api

MAKE_INSTALL_FLAGS = PKG_INSTALL_DIR="$(PKG_INSTALL_DIR)"
CONFIGURE_VARS=
LDFLAGS += -L$(BUILD_DIR)/libcurl/pkg-install/lib
LDFLAGS += -L$(BUILD_DIR)/json-c/pkg-install/lib
CFLAGS += -I$(BUILD_DIR)/libcurl/pkg-install/include 
CFLAGS += -I$(BUILD_DIR)/json-c/pkg-install/include 
CFLAGS += $(EXT_CFLAGS)
LDFLAGS += $(EXT_LDFLAGS)

define Package/es-c-api
  DEPENDS:=libs/json-c libs/curl
endef

define Package/es-c-api/Prepare
	@echo "pakcage prepare:"
	-mkdir -p $(BUILD_PATH)
	@if [ ! -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
		cp -r $(SRC_DIR) $(BUILD_PATH); \
	fi
endef

define Package/es-c-api/Install
	@echo "package install"
	$(INSTALL_DIR) $(TARGET_DIR)/lib
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/lib/libesc.so $(TARGET_DIR)/lib
endef


export CFLAGS LDFLAGS
$(eval $(call BuildPackage,es-c-api))
