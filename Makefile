CC ?= gcc
CXX ?= g++
EXTRA_CFLAGS ?=
EXTRA_LDFLAGS ?=
CFLAGS := $(shell pkg-config --cflags glib-2.0 gio-2.0 gtk+-3.0 libxml-2.0) -Wall -g -ansi -std=c99 $(EXTRA_CFLAGS)
CXXFLAGS := $(shell pkg-config --cflags glib-2.0 gio-2.0 gtk+-3.0 libxml-2.0) -Wall -g -ansi $(EXTRA_CFLAGS) -D__STDC_LIMIT_MACROS
LDFLAGS = $(EXTRA_LDFLAGS) -Wl,--as-needed
LDADD := $(shell pkg-config --libs glib-2.0 gio-2.0 gtk+-3.0 gthread-2.0 alsa libxml-2.0) -lexpat -lm -lrtmidi
OBJECTS = gdigi.o gui.o effects.o preset.o gtkknob.o preset_xml.o resources.o gtkapp.o rtmidi.o
DEPFILES = $(foreach m,$(OBJECTS:.o=),.$(m).m)



.PHONY : clean distclean all
%.o : %.c
	$(CC) $(CFLAGS) -c $<

.%.m : %.c
	$(CC) $(CFLAGS) -M -MF $@ -MG $<

all: gdigi

rtmidi.o: rtmidi.cpp
	$(CXX) $(CXXFLAGS) -c $<

gdigi: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $+ $(LDADD)

resources.c: resources.gresource.xml images/gdigi.png images/icon.png images/knob.png menus.ui
	glib-compile-resources --generate-source --c-name gdigi resources.gresource.xml --target=$@

resources.h: resources.gresource.xml images/gdigi.png images/icon.png images/knob.png menus.ui
	glib-compile-resources --generate-header --c-name gdigi resources.gresource.xml --target=$@

clean:
	rm -f *.o resources.c resources.h

distclean : clean
	rm -f .*.m
	rm -f resources.c
	rm -f gdigi

install: gdigi
	install gdigi $(DESTDIR)/usr/bin
	install -m 0644 gdigi.desktop $(DESTDIR)/usr/share/applications/
	install -m 0644 images/gdigi.png $(DESTDIR)/usr/share/icons/

NODEP_TARGETS := clean distclean
depinc := 1
ifneq (,$(filter $(NODEP_TARGETS),$(MAKECMDGOALS)))
depinc := 0
endif
ifneq (,$(fitler-out $(NODEP_TARGETS),$(MAKECMDGOALS)))
depinc := 1
endif

ifeq ($(depinc),1)
-include $(DEPFILES)
endif
