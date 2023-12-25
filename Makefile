
WESTON_MODPREFIX=$(shell pkg-config --variable=libexecdir weston)
WESTON_VERSION=$(shell pkg-config --modversion weston | cut -d '.' -f 1)

LIBWESTON_MOD=libweston-$(WESTON_VERSION)
LIBWESTON_LIBPREFIX=$(shell pkg-config --variable=libdir $(LIBWESTON_MOD))

CFLAGS :=$(CFLAGS)\
	$(shell pkg-config --cflags weston $(LIBWESTON_MOD)) \
	$(shell pkg-config --cflags libevdev) \
	 -fPIC -Wall

LDFLAGS:=$(LDFLAGS) -shared $(shell pkg-config --libs $(LIBWESTON_MOD))

.PHONY: all clean install uninstall

all: binder.so

binder.so: binder.c Makefile $(LIBWESTON_LIBPREFIX)/$(LIBWESTON_MOD).so
	${CC} ${CFLAGS} $< ${LDFLAGS} -o $@

clean:
	rm -f binder.so

install:
	sudo install -m 755 binder.so $(WESTON_MODPREFIX)

uninstall:
	sudo rm -f $(WESTON_MODPREFIX)/binder.so
