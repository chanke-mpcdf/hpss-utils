#
# Define HPSS for 6.2 and above systems
#
LOCAL_ROOT    = ..
LOCAL_LIB     = $(LOCAL_ROOT)/lib
DESTDIR       = $(LOCAL_ROOT)/bin

export LD_LIBRARY_PATH = $(LOCAL_LIB):$(LOCAL_ROOT)/tools/unsupported/lib

include $(LOCAL_ROOT)/Makefile.macros

COMPFLAGS      = -g

#
# Common
                                                                                
INCLUDE_PATH	= -I. -I$(LOCAL_INCLUDE) 
CFLAGS		= $(INCLUDE_PATH) $(COMPFLAGS) $(MACHINE_FLAGS)

.c.o:;		@echo "Compiling $<"...
		@$(CC) $(CFLAGS) -c $<

LIBS		= -L$(LOCAL_LIB) -lhpss -lhpsscs

POSIXLIB	= -L$(LOCAL_ROOT)/tools/unsupported/lib/ \
		  -lhpssposix

LDFLAGS         = $(COMPFLAGS) $(RUNFLAGS)
PROG		= hpss_mkdir hpss_rmdir hpss_rm hpss_purge hpss_symlink\
		  hpss_hardlink hpss_chown hpss_ls hpss_put hpss_cat\
		  hpss_readdata hpss_putdata hpss_touch hpss_chmod\
		  hpss_getvol hpss_catbuffered hpss_putbuffered\
	          hpss_putposix hpss_catposix hpss_posixstat hpss_stat\
		  hpss_buffertest hpss_recursive_ls hpss_rename 

all:		$(PROG)
install:	all
		@mkdir -p $(DESTDIR)
		@for prog in $(PROG); do \
		  install -m 0755 $$prog $(DESTDIR); \
		done  

clean:;		/bin/rm -f *.o $(PROG)


hpss_putposix.o:        
		$(CC) -c -I$(LOCAL_ROOT)/tools/unsupported/include $(CFLAGS) $(LDFLAGS) $(LIBS) hpss_putposix.c

hpss_catposix.o:        
		$(CC) -c -I$(LOCAL_ROOT)/tools/unsupported/include $(CFLAGS) $(LDFLAGS) $(LIBS) hpss_catposix.c

hpss_posixstat.o:        
		$(CC) -c -I$(LOCAL_ROOT)/tools/unsupported/include $(CFLAGS) $(LDFLAGS) $(LIBS) hpss_posixstat.c

hpss_mkdir:	hpss_mkdir.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_rmdir:	hpss_rmdir.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_rm:		hpss_rm.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_purge:	hpss_purge.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_symlink:	hpss_symlink.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_hardlink:	hpss_hardlink.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_chown:	hpss_chown.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_ls:		hpss_ls.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ -g $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_ls_dlopen: hpss_ls_dlopen.o tools.o
		@$(CC) $@.o -o $@ -g $(CFLAGS) -ldl tools.o

hpss_put:	hpss_put.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_cat:	hpss_cat.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_readdata:	hpss_readdata.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_putdata:	hpss_putdata.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_touch:	hpss_touch.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_test2:	hpss_test2.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

hpss_test3:	hpss_test3.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS)

hpss_chmod:	hpss_chmod.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_getvol:	hpss_getvol.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_catbuffered:	hpss_catbuffered.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_putbuffered:	hpss_putbuffered.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_putposix:	hpss_putposix.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) $(POSIXLIB) tools.o

hpss_catposix:	hpss_catposix.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) $(POSIXLIB) tools.o

hpss_posixstat:	hpss_posixstat.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) $(POSIXLIB) tools.o

hpss_stat:	hpss_stat.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_buffertest:	hpss_buffertest.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_recursive_ls:	hpss_recursive_ls.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

localputdata:	localputdata.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o

hpss_rename:	hpss_rename.o tools.o
		@echo "Linking $@"...
		@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) tools.o
