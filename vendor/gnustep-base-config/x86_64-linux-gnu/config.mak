#
# Extra make variables for base library
#
# Copyright (C) 2005-2010 Free Software Foundation
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

WHOAMI=/usr/bin/whoami

DYNAMIC_LINKER=simple

HAVE_LIBXML=1
HAVE_GNUTLS=1
HAVE_BLOCKS=1

WITH_FFI=libffi

NX_CONST_STRING_CLASS=NSConstantString
OBJCFLAGS+=
OBJC2RUNTIME=1
OBJCSYNC=1
WARN_FLAGS=-Wall -Wdeclaration-after-statement

HAVE_INET_PTON=yes
HAVE_INET_NTOP=yes
HAVE_OBJC_SYNC_ENTER=yes

CONFIG_SYSTEM_INCL +=  -I/usr/GNUstep/Local/Library/Headers -I/usr/GNUstep/Local/Library/Headers -I/usr/GNUstep/System/Library/Headers -I/usr/GNUstep/Local/Library/Headers  -I/usr/include/libxml2 -DWITH_GZFILEOP -I/usr/include/libxml2 -DWITH_GZFILEOP -I/usr/include/p11-kit-1
ifeq ($(shared),yes)
  CONFIG_SYSTEM_LIBS += -lgmp   -lgnutls -lxslt -lxml2 -lxml2 -lffi -lz -lrt -ldl  -lpthread -lz -licui18n -ldispatch -lcurl
  CONFIG_SYSTEM_LIB_DIR +=  -L/usr/GNUstep/Local/Library/Libraries -L/usr/GNUstep/Local/Library/Libraries -L/usr/GNUstep/System/Library/Libraries -L/usr/GNUstep/Local/Library/Libraries/ng-gnu-gnu -L/usr/GNUstep/Local/Library/Libraries
endif

GNUSTEP_INSTALL_GDOMAP_AS_SETUID=no
GNUSTEP_GDOMAP_PORT_OVERRIDE=no

GNUSTEP_BASE_HAVE_AVAHI=0
GNUSTEP_BASE_HAVE_GNUTLS=1
GNUSTEP_BASE_HAVE_ICU=1
GNUSTEP_BASE_HAVE_LIBCURL=1
GNUSTEP_BASE_HAVE_LIBDISPATCH=1
GNUSTEP_BASE_HAVE_LIBDISPATCH_RUNLOOP=1
GNUSTEP_BASE_HAVE_LIBXML=1
GNUSTEP_BASE_HAVE_MDNS=0

# Default to building only -baseadd
# on non *-gnu-* library combos
ifneq ($(FOUNDATION_LIB),gnu)
add=yes
base=no
endif

GNUSTEP_BASE_DOMAIN=LOCAL
GNUSTEP_BASE_RELATIVE_PATHS=no

# Any global prefprocessor defines can be added here
DEFS=

ifeq ($(GNUSTEP_BASE_RELATIVE_PATHS), yes)
ifneq ($(GNUSTEP_BASE_DOMAIN), $(GNUSTEP_INSTALLATION_DOMAIN))
$(warning "Error: GNUSTEP_INSTALLATION_DOMAIN does not match GNUSTEP_BASE_DOMAIN")
$(warning " ")
$(warning "You can install gnustep-base in one of the four domains: SYSTEM, LOCAL, NETWORK or USER.")
$(warning " ")
$(warning "gnustep-base was configured to be installed into: $(GNUSTEP_BASE_DOMAIN)")
$(warning "but it would now being installed into: $(GNUSTEP_INSTALLATION_DOMAIN)")
$(warning "That's not right: they must match.")
$(warning " ")
$(warning "If you want to install into $(GNUSTEP_BASE_DOMAIN), please use")
$(warning " ")
$(warning "   make install GNUSTEP_INSTALLATION_DOMAIN=$(GNUSTEP_BASE_DOMAIN) [plus any other options you need]")
$(warning " ")
$(warning "Instead, if you want to install into $(GNUSTEP_INSTALLATION_DOMAIN), please reconfigure")
$(warning "gnustep-base by doing")
$(warning " ")
$(warning "   ./configure --with-installation-domain=$(GNUSTEP_INSTALLATION_DOMAIN) [plus any other options you need]")
$(warning " ")
$(warning "and then recompile and reinstall.")

# We used to automatically rerun configure here.  Unfortunately we
# don't know if the right domain that we should be installing into is
# GNUSTEP_BASE_DOMAIN (specified or automatically picked up by
# configure) or GNUSTEP_INSTALLATION_DOMAIN (specified or
# automatically picked up by make install) so the user really has to
# spend two minutes checking this and fixing it herself.

$(error "Please fix the installation domain then try again.  If you are stuck, contact discuss-gnustep@gnu.org for help.")
endif
endif
