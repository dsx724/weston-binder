PREFIX ?= /usr
LIBDIR = $(PREFIX)/lib
MODULEDIR = $(LIBDIR)/weston

CFLAGS :=$(CFLAGS) \
	$(shell pkg-config --cflags weston) \
	$(shell pkg-config --cflags libevdev)

LDFLAGS :=$(LDFLAGS) -shared -lweston-14

.PHONY: all clean install uninstall

all: binder.so

binder.so: binder.o
	$(CC) $(LDFLAGS) -o $@ $^

binder.o: binder.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f binder.so binder.o

install: all
	install -d $(DESTDIR)$(MODULEDIR)
	install -m 0644 binder.so $(DESTDIR)$(MODULEDIR)
