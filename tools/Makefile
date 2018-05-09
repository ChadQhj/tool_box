include $(TOP_DIR)/rules.mk
ALL_TOOLS=$(shell ls ./ -F | grep /$)
BUILD_TOOL_DIR=


define add_build_dir
  #UPPER_DIR := $(shell echo -n $(1) | tr a-z A-Z)
  ifneq ($(COMPILE_$(1)),n)
    BUILD_TOOL_DIR+=$(1)
  endif
endef

define compile_tools
	@echo "compile_tools:"$(BUILD_TOOL_DIR)
	$(foreach dir,$(BUILD_TOOL_DIR),
		$(MAKE) -C $(dir)
	)
endef

define clean_tools
	@echo "clean_tools:"$(BUILD_TOOL_DIR)
	$(foreach dir,$(BUILD_TOOL_DIR),
		$(MAKE) -C $(dir) clean
	)
endef

define distclean_tools
	@echo "distclean_tools:"$(BUILD_TOOL_DIR)
	$(foreach dir,$(BUILD_TOOL_DIR),
		$(MAKE) -C $(dir) distclean
	)
endef

$(foreach tool,$(ALL_TOOLS),$(eval $(call add_build_dir,$(tool))))

all:
	$(call compile_tools)
	@echo "compile_tools done,success"
	
clean:
	$(call clean_tools)
	@echo "clean_tools done,success"

distclean:
	$(call distclean_tools)
	@echo "distclean_tools done,success"
