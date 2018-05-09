include $(TOP_DIR)/rules.mk
include $(INCLUDE_DIR)/default.mk

SRC_DIR=json-c-0.13
MAKE_PATH=json-c-0.13
CONFIGURE_PATH=json-c-0.13
BUILD_PATH=$(BUILD_DIR)/json-c
PRE_CONFIGURE=./autogen.sh
CONFIGURE_VARS = --enable-threading

MAKE_INSTALL_FLAGS = "PKG_INSTALL_DIR=$(BUILD_PATH)/pkg-install"

define Package/json-c
  DEPENDS:=
endef

define Package/json-c/Prepare
	@echo "pakcage prepare:"
	-mkdir -p $(BUILD_PATH);
	@if [ ! -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
		cp -r $(SRC_DIR) $(BUILD_PATH); \
	fi
endef

define Package/json-c/Install
	@echo "package install"
	$(INSTALL_DIR) $(TARGET_DIR)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/bin/* $(TARGET_DIR)/usr/bin/
	$(INSTALL_DIR) $(TARGET_DIR)/etc
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/etc/* $(TARGET_DIR)/etc
endef

$(eval $(call BuildPackage,json-c))
