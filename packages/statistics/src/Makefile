CC = g++
CFLAGS += -I/usr/include/mysql/
LDFLAGS +=
BINDIR = $(PKG_INSTALL_DIR)/usr/bin
LDLIBS += -lpthread -lmysqlclient -lheartbeat

SRC_FILES = statistic_sql.c statistic.c debug.c

TARGET = statistic

all: $(TARGET)

statistic: $(SRC_FILES)
	echo $(LDFLAGS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LDLIBS)

install: uninstall all
	-mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -rf *.o *~ .*sw* $(TARGET)

