#use your package instead of example
#step1:configure
#	$(PRE_CONFIGURE)
#	$(CONFIGURE) $(DEF_CONFIGURE_VARS) $(CONFIGURE_VARS)
#step2:make
#	$(MAKE) $(MAKE_VARS)
#step3:install
#	$(MAKE) $(MAKE_INSTALL_FLAGS) install
#

include $(TOP_DIR)/rules.mk
include $(INCLUDE_DIR)/default.mk

#src is the default directory,modify it to the correct name if the name of dircetory is not src
SRC_PATH=src
MAKE_PATH=src
CONFIGURE_PATH=src
BUILD_PATH=$(BUILD_DIR)/example

#overide values below if necessary
#PRE_CONFIGURE=
#DEF_CONFIGURE_VARS=--prefix=$(PKG_INSTALL_DIR)
#CONFIGURE_VARS=
#MAKE_VARS=
#MAKE_INSTALL_FLAGS=


#define values
define Package/example
  DEPENDS:=
endef

define Package/example/Prepare
	@echo "pakcage prepare:"
	-mkdir -p $(BUILD_PATH)
	@if [ ! -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
		cp -r $(SRC_PATH) $(BUILD_PATH); \
	fi
endef

define Package/example/Install
	@echo "package install"
endef


$(eval $(call BuildPackage,example))
