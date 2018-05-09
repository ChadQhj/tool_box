
all:compile
	
define Build/Install
	@echo "Build/Install**********************************************"
	-mkdir -p $(PKG_INSTALL_DIR)
	cd $(BUILD_PATH)/$(MAKE_PATH);$(MAKE) $(MAKE_INSTALL_FLAGS) install
endef

define Build/Compile
	@echo "Build/Compile**********************************************"
	cd $(BUILD_PATH)/$(MAKE_PATH);$(MAKE) $(MAKE_VARS)
endef

define Build/Configure
	cd $(BUILD_PATH)/$(CONFIGURE_PATH);chmod +x $(CONFIGURE);$(PRE_CONFIGURE);./$(CONFIGURE) $(DEF_CONFIGURE_VARS) $(CONFIGURE_VARS)
endef

define Build/Installfs
	@echo "Build/Installfs**********************************************"
	@$(INSTALL_DIR) $(TARGET_DIR)
	@if [ $(shell ls $(PKG_INSTALL_DIR) | wc -l) -gt 0 ]; then \
		cp -rf $(PKG_INSTALL_DIR)/* $(TARGET_DIR)/; \
	fi
endef

define Build/Default
compile:
	@echo "Build/Default**********************************************"$(1)
ifneq ($(DEPENDS),)
	$(call Build/Depends)
endif
	$(call Package/$(1)/Prepare)
	@if [ -f $(BUILD_PATH)/$(MAKE_PATH)/$(MAKEFILE_NAME) ]; then \
		echo "makefile existent,compile directly=============================="; \
	else \
		echo "no makefile found,excute configure"; \
		$(call Build/Configure); \
	fi
	$(call Build/Compile)
	$(call Build/Install)
install:
	$(call Package/$(1)/Install)
	#$(call Build/Installfs)
endef

define Build/Depends
	@echo "Build/Depends**********************************************"$(TOP_DIR)/$(depend)
	$(foreach depend,$(DEPENDS),
		$(MAKE) -C $(TOP_DIR)/$(depend)
	)
endef

define Package/Default
  CONFIGFILE:=
  SECTION:=opt
endef

define BuildPackage
  $(eval $(Package/Default))
  $(eval $(Package/$(1)))
  $(eval $(call Build/Default,$(1)))
endef

pre-compile:
	@echo "pre-compile done"

compile:pre-compile


clean:
	@if [ -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
		cd $(BUILD_PATH)/$(MAKE_PATH);$(MAKE) clean; \
	fi
	
distclean:
	#@if [ -d $(BUILD_PATH)/$(MAKE_PATH) ]; then \
	#	cd $(BUILD_PATH)/$(MAKE_PATH);$(MAKE) distclean; \
	#fi
	rm -fr $(BUILD_PATH)
	
.PHONY: clean distclean
	


