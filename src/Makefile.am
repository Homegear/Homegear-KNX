AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
LIBS += -Wl,-Bdynamic -lzip

libdir = $(localstatedir)/lib/homegear/modules
lib_LTLIBRARIES = mod_knx.la
mod_knx_la_SOURCES = Knx.cpp KnxPeer.cpp Search.cpp DptConverter.cpp Factory.cpp Cemi.cpp KnxIpForwarder.cpp KnxIpPacket.cpp GD.cpp KnxCentral.cpp Interfaces.cpp PhysicalInterfaces/MainInterface.cpp
mod_knx_la_LDFLAGS =-module -avoid-version -shared
install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/mod_knx.la
