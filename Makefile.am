# $Id: Makefile.am,v 1.2 2009/12/15 20:03:38 tyreld Exp $ 
# ==================================================================
# (C) Copyright IBM Corp. 2005
#
# THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
# ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
# CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
#
# You can obtain a current copy of the Common Public License from
# http://www.opensource.org/licenses/cpl1.0.php
#
# Author:       Tyrel Datwyler <tyreld@us.ibm.com>
# Description:  Automake input file for OS Base Service Providers
# ==================================================================


# SBLIM Include Directory
sblimincdir = $(includedir)/sblim

# Start from an empty extra distribution file list
EXTRA_DIST=

# docdir defines where the documentation goes
docdir=$(datadir)/doc/$(PACKAGE)-$(VERSION)

#
# Automake instructions for documentation
#
doc_DATA=README AUTHORS COPYING 

# ADD EXTRA DOC FILES IF PRESENT
#doc_DATA+=DEBUG README.tog-pegasus
#EXTRA_DIST+=DEBUG README.tog-pegasus

# providerdir defines where provider libraries will be installed (@PROVIDERDIR@ is set by the configure script). 
providerdir = @PROVIDERDIR@

# Automake instructions for main dir

#  Service CMPI provider libraries
provider_LTLIBRARIES = 	libcmpiOSBase_ServiceProvider.la \
                        libcmpiOSBase_HostedServiceProvider.la \
                        libcmpiOSBase_ServiceProcessProvider.la

# Service
libcmpiOSBase_ServiceProvider_la_SOURCES = \
        cmpiOSBase_ServiceProvider.c \
        cmpiOSBase_Service.c
libcmpiOSBase_ServiceProvider_la_LIBADD = \
        -lcmpiOSBase_Common \
        -lcimlxs
        
# HostedService
libcmpiOSBase_HostedServiceProvider_la_SOURCES = \
        cmpiOSBase_HostedServiceProvider.c
libcmpiOSBase_HostedServiceProvider_la_LIBADD = \
        -lcmpiOSBase_Common \
        -lcimlxs
        
# ServiceProcess
libcmpiOSBase_ServiceProcessProvider_la_SOURCES = \
        cmpiOSBase_ServiceProcessProvider.c \
        cmpiOSBase_ServiceProcess.c
libcmpiOSBase_ServiceProcessProvider_la_LIBADD = \
        -lcmpiOSBase_Common \
        -lcimlxs
        
# Support Utility Library
lib_LTLIBRARIES = libcimlxs.la

libcimlxs_la_SOURCES = cimlxs.c
libcimlxs_la_LIBADD = -lcmpiOSBase_Common 

# Installable Header Files 
sbliminc_HEADERS = cimlxs.h

# Non-Installable Header Files
noinst_HEADERS = \
        cmpiOSBase_Service.h \
        cmpiOSBase_ServiceProcess.h

# Non-Installable Programs
noinst_PROGRAMS = svclist
svclist_SOURCES = svclist.c
svclist_LDADD = -lcimlxs -lcmpiOSBase_Common

dist_bin_SCRIPTS = listservices

# We must explicity add the RPM spec files to the distribution package
EXTRA_DIST+=$(PACKAGE).spec $(PACKAGE).rh.spec

# Automake instructions for ./mof subdir

SCHEMAS=$(srcdir)/mof/Linux_Service.mof
REGISTRATIONS=$(srcdir)/mof/Linux_Service.registration 

# We must explicity add all the schema files to the distribution package
pkgdata_DATA=$(SCHEMAS) $(REGISTRATIONS)
pkgdata_SCRIPTS=provider-register.sh
EXTRA_DIST+=mof $(pkgdata_SCRIPTS)

# Register the provider(s) and class definition(s) to the current CIM server/CIMOM
postinstall:
	sh $(srcdir)/provider-register.sh -t @CIMSERVER@ -r $(REGISTRATIONS) -m $(SCHEMAS)

preuninstall:
	sh $(srcdir)/provider-register.sh -d -t @CIMSERVER@ -r $(REGISTRATIONS) -m $(SCHEMAS)

dist-hook:
	test -d "$(distdir)" &&	rm -rf `find $(distdir) -type d -name CVS`
