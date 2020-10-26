#------------------------------------------------------------------------------
# Compiler flags
#------------------------------------------------------------------------------
CFLAGS += -O1 -g3  -Wall
#CFLAGS += -O2 -g   -Wsign-compare -Wfloat-equal -Wformat-security #-Werror

CFLAGS += -I$(SYSROOT)/usr/include/OsLinux/
CFLAGS += -I$(SYSROOT)/usr/include/dal/

#------------------------------------------------------------------------------
# Linker flags
#------------------------------------------------------------------------------
LDFLAGS += -ldal -llibloader -lpthread -lffi -lrt -ldbus-glib-1 -lglib-2.0 -lm
LDFLAGS += -ltypelabel -loslinux -ldbuskbuscommon -lpaho-mqtt3as -lprotobuf-c

OBJECTS := powerreader.o protobuf/result_set.pb-c.o
EXECUTABLE := powerreader

all: powerreader

powerreader: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

clean:
	$(RM) $(OBJECTS) $(EXECUTABLE)

install:

.PHONY: all install clean
