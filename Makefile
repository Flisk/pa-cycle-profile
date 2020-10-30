CFLAGS := \
	`pkg-config --cflags --libs libpulse` \
	-Wextra \
	-Wall \
	-Werror=format-security \
	-Werror=implicit-function-declaration \
	$(CFLAGS)

PREFIX ?= /usr/local
BINDIR := $(DESTDIR)$(PREFIX)/bin

pa-cycle-profile: pa-cycle-profile.c

install: pa-cycle-profile
	install -D $< $(BINDIR)/$<

uninstall:
	rm $(BINDIR)/pa-cycle-profile

