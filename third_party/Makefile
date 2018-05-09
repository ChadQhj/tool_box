include $(TOP_DIR)/rules.mk
ALL_PKGS=$(shell ls ./ -F | grep /$)
BUILD_PKG_DIR=


define add_build_dir
  ifneq ($(COMPILE_$(1)),n)
    BUILD_PKG_DIR+=$(1)
  endif
endef

define compile_dirs
	@echo "compile_dirs:"$(BUILD_PKG_DIR)
	$(foreach dir,$(BUILD_PKG_DIR),
		$(MAKE) -C $(dir)
	)
endef

define install_dirs
	@echo "install_dirs:"$(BUILD_PKG_DIR)
	$(foreach dir,$(BUILD_PKG_DIR),
		$(MAKE) -C $(dir) install
	)
endef

define clean_dirs
	@echo "clean_dirs:"$(BUILD_PKG_DIR)
	$(foreach dir,$(BUILD_PKG_DIR),
		$(MAKE) -C $(dir) clean
	)
endef

define distclean_dirs
	@echo "distclean_dirs:"$(BUILD_PKG_DIR)
	$(foreach dir,$(BUILD_PKG_DIR),
		$(MAKE) -C $(dir) distclean
	)
endef

$(foreach pkg,$(ALL_PKGS),$(eval $(call add_build_dir,$(pkg))))

all:
	$(call compile_dirs)
	@echo "compile_dirs done,success"
	
install:
	$(call install_dirs)
	@echo "install_dirs done,success"
	
clean:
	$(call clean_dirs)
	@echo "clean_dirs done,success"

distclean:
	$(call distclean_dirs)
	@echo "distclean_dirs done,success"
