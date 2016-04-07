#
# Define HPSS for 6.2 and above systems
#
LOCAL_ROOT    = /opt/hpss
ROOT_SUBPATH  = .

include $(LOCAL_ROOT)/Makefile.macros

COMPFLAGS      = -O0 -g -Wall -Werror

                                                                                
INCLUDE_PATH	= -I. -I$(LOCAL_INCLUDE) -I$(DB_INSTALL_PATH)/include
CFLAGS		= $(INCLUDE_PATH) $(COMPFLAGS) $(MACHINE_FLAGS)

.c.o:;		@echo "Compiling $<"...
		@$(CC) $(CFLAGS) -c $<

LIBS		= -L$(BUILD_LIB) \
                  -lhpss -lhpsscs -levent

POSIXLIB	= -L/opt/hpss/tools/unsupported/lib/ \
		  -lhpssposix

LDFLAGS         = $(COMPFLAGS) $(RUNFLAGS)
OBJ		= util.o utf8.o hpss_mkdir.o hpss_chmod.o hpss_rm.o hpss_ls.o \
		  hpss_put_from_proxy.o hpss_get_to_proxy.o hpss_link.o hpss_stat.o \
		  hpss_rename.o hpss_set_uda.o hpss_get_storage_info.o hpss_chown.o\
		  hpss_get_uda.o hpss_del_uda.o

PROG		= hpss-http_proxy

all:		$(PROG)

clean:;		/bin/rm -f *.o $(PROG)


hpss-http_proxy:  hpss-http_proxy.o $(OBJ)
	@echo "Linking $@"...
	@$(CC) $@.o -o $@ $(CFLAGS) $(LDFLAGS) $(LIBS) $(OBJ)

