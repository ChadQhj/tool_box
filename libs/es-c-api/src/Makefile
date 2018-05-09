shared-lib:
	gcc es_action.c -fPIC -shared -o libesc.so $(CFLAGS) $(LDFLAGS) -ljson-c -lcurl

install:
	mkdir -p $(PKG_INSTALL_DIR)/lib
	mkdir -p $(PKG_INSTALL_DIR)/include
	cp libesc.so $(PKG_INSTALL_DIR)/lib
	cp es_action.h $(PKG_INSTALL_DIR)/include
	
