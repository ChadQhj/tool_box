
include pre-build.mk
include rules.mk

#DEFAULT_BUILD_DIR = libs packages third_party
DEFAULT_BUILD_DIR = packages third_party

define build_project
	@echo "build_project..."
	@echo "mkdir build_dir"
        -mkdir -p -m 777 $(BUILD_DIR)
	$(foreach build_dir,$(DEFAULT_BUILD_DIR),
		$(MAKE) -C $(TOP_DIR)/$(build_dir)
	)
endef

define install_project
	@echo "install_project..."
	$(foreach build_dir,$(DEFAULT_BUILD_DIR),
		$(MAKE) -C $(TOP_DIR)/$(build_dir) install
	)
endef

define clean_project
	@echo "clean_project..."
	$(foreach build_dir,$(DEFAULT_BUILD_DIR),
		$(MAKE) -C $(TOP_DIR)/$(build_dir) clean
	)
endef

define distclean_project
	@echo "distclean_project..."
	$(foreach build_dir,$(DEFAULT_BUILD_DIR),
		$(MAKE) -C $(TOP_DIR)/$(build_dir) distclean
	)
endef

all:
	$(call build_project)
	@echo "build_project done,success"
	
prepare:
	@if [ ! -f ./.ready_to_compile ]; then \
		ln -s /usr/include/dumbnet.h /usr/include/dnet.h 2>/dev/null; \
		cp ./script/daq-modules-config /usr/bin/; \
		touch ./.ready_to_compile; \
	fi

clean:
	$(call clean_project)
	@echo "clean_project done,success"

distclean:
	$(call distclean_project)
	@rm -fr $(BUILD_DIR)/*
	@echo "distclean_project done,success"

install:
	$(call install_project)
	-mkdir -p target/rootfs/etc
	@cp include/ids-ld-so.conf target/rootfs/etc/
	@cd target;tar -zcvf ids.tar.gz rootfs/
	@echo "install_project done,success"
	
uninstall:
	@rm -fr $(TARGET_DIR)/*
	@rm -fr $(SHARED_DIR)/*
	@rm -f target/ids.tar.gz
	@echo "uninstall done,success"
