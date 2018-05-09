include $(TOP_DIR)/rules.mk
include $(INCLUDE_DIR)/default.mk

SRC_DIR=curl-7.56.1
MAKE_PATH=curl-7.56.1
CONFIGURE_PATH=curl-7.56.1
BUILD_PATH=$(BUILD_DIR)/libcurl

MAKE_INSTALL_FLAGS = "PKG_INSTALL_DIR=$(BUILD_PATH)/pkg-install"

define Package/libcurl
  DEPENDS:=
endef

define Package/libcurl/Prepare
	@echo "pakcage prepare:"
	-mkdir -p $(BUILD_PATH);
	@if [ ! -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
		cp -r $(SRC_DIR) $(BUILD_PATH); \
	fi
endef

define Package/libcurl/Install
	@echo "package install"
	$(INSTALL_DIR) $(TARGET_DIR)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/local/bin/* $(TARGET_DIR)/usr/bin/
	$(INSTALL_DIR) $(TARGET_DIR)/etc
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/etc/* $(TARGET_DIR)/etc
endef

$(eval $(call BuildPackage,libcurl))
