# Makefile for SBLIM (/etc/init.d) Service Instrumentation using the 
# Common Manageability Programming Interface (CMPI)

include setting.cmpi

.PHONY: all testfiles install clean uninstall

#------------------------------------------------------------------------------#

# No changes necessary below this line
CFLAGS=-Wall -g -I. -I$(CIMOMINC) -I$(COMMONINC) -D_COMPILE_UNIX -DDEBUG -DNOEVENTS -fPIC
LDFLAGS=-L. -L$(CIMOMLIB) -L$(COMMONLIB) -shared -lcmpiOSBase_Common


lib%.so: %.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $?


all: 	testfiles \
	libcimlxs.so \
	libcmpiOSBase_ServiceProvider.so \
	libcmpiOSBase_HostedServiceProvider.so \
	libcmpiOSBase_ServiceProcessProvider.so 


cimlxs.c: cimlxs.h


libcmpiOSBase_ServiceProvider.so: cmpiOSBase_ServiceProvider.c \
				  cmpiOSBase_Service.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -lcimlxs -o $@ $^


libcmpiOSBase_HostedServiceProvider.so: cmpiOSBase_HostedServiceProvider.c 
	$(CC) $(CFLAGS) $(LDFLAGS) -lcimlxs -o $@ $^


libcmpiOSBase_ServiceProcessProvider.so: cmpiOSBase_ServiceProcessProvider.c \
				         cmpiOSBase_ServiceProcess.c
	$(CC) $(CFLAGS) $(LDFLAGS) -lcimlxs -o $@ $^


svclist: LDFLAGS = -L $(CIMOMLIB) -lcimlxs -lcmpiOSBase_Common -ldl


#------------------------------------------------------------------------------#


install: all
	install listservices /usr/local/bin
	install libcimlxs.so $(CIMOMLIB)
	install libcmpiOSBase_*Provider.so $(CIMOMLIB)
	$(MAKE) -C mof -f $(MOFMAKEFILE) install


testfiles:
	@[ -d $(CIMOMLIB) ] || ( echo directory $(CIMOMLIB) does not exist && false)
	@[ -d $(CIMOMINC) ] || ( echo directory $(CIMOMINC) does not exist - please create manually && false)
	@[ -d $(CIMOMMOF) ] || ( echo directory $(CIMOMMOF) does not exist - please create manually && false)
	@[ -d $(COMMONINC) ] || ( echo directory $(COMMONINC) does not exist - please create manually && false)
	@[ -d $(COMMONLIB) ] || ( echo directory $(COMMONLIB) does not exist - please create manually && false)


clean:
	$(RM)	*.so *.o *~ svclist


uninstall: 
	$(MAKE) -C mof -f $(MOFMAKEFILE) uninstall;
	$(RM) /usr/local/bin/listservices \
	$(RM) $(CIMOMLIB)/libcimlxs.so \
	$(RM) $(CIMOMLIB)/libcmpiOSBase_ServiceProvider.so \
	$(RM) $(CIMOMLIB)/libcmpiOSBase_HostedServiceProvider.so \
	$(RM) $(CIMOMLIB)/libcmpiOSBase_ServiceProcessProvider.so
