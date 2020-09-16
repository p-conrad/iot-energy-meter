#------------------------------------------------------------------------------
# Compiler flags
#------------------------------------------------------------------------------
CFLAGS += -O0 -g3  -Wall
#CFLAGS += -O2 -g   -Wsign-compare -Wfloat-equal -Wformat-security #-Werror

CFLAGS += -I$(SYSROOT)/usr/include/OsLinux/
CFLAGS += -I$(SYSROOT)/usr/include/dal/

#------------------------------------------------------------------------------
# Linker flags
#------------------------------------------------------------------------------
LDFLAGS += -ldal -llibloader -lpthread -lffi -lrt -ldbus-glib-1 -lglib-2.0 
LDFLAGS += -ltypelabel -loslinux -ldbuskbuscommon


all: powerreader

powerreader: powerreader.o
	$(CC) $< -o $@ $(LDFLAGS)

clean:
	-rm -f powerreader powerreader.o

install:

.PHONY: all install clean
