CC = gcc
LDFLAGS +=
BINDIR = $(PKG_INSTALL_DIR)/usr/bin
LDLIBS += -lheartbeat -lpthread -ljson-c -lesc

SRC_FILES = alerting.c debug.c

TARGET = alerting

all: $(TARGET)

alerting: $(SRC_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

install: uninstall all
	-mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -rf *.o *~ .*sw* $(TARGET)

