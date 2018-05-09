include $(TOP_DIR)/rules.mk
include $(INCLUDE_DIR)/default.mk

SRC_DIR=src
MAKE_PATH=src
CONFIGURE_PATH=src
BUILD_PATH=$(BUILD_DIR)/alerting

MAKE_INSTALL_FLAGS = PKG_INSTALL_DIR="$(PKG_INSTALL_DIR)"
CONFIGURE_VARS=
LDFLAGS += $(EXT_LDFLAGS) -L$(SHARED_DIR)/usr/lib
CFLAGS += $(EXT_CFLAGS) -I$(SHARED_DIR)/usr/include -I$(BUILD_DIR)/es-c-api/pkg-install/include -I$(BUILD_DIR)/libcurl/pkg-install/include

define Package/alerting
  #DEPENDS:=libs/heartbeat third_party/mailx
  DEPENDS:=third_party/mailx libs/es-c-api
endef

define Package/alerting/Prepare
	@echo "pakcage prepare:"
	-mkdir -p $(BUILD_PATH)
	@if [ ! -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
		cp -r $(SRC_DIR) $(BUILD_PATH); \
	fi
endef

define Package/alerting/Install
	@echo "package install"
	$(INSTALL_DIR) $(PKG_INSTALL_DIR)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(TARGET_DIR)/usr/bin/
endef


export CFLAGS LDFLAGS
$(eval $(call BuildPackage,alerting))
