ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

CCFLAGS += -g -O0
LDFLAGS +=  -M
LIBS += usbdi screen hiddi

NAME = usb-to-screen

include $(MKFILES_ROOT)/qtargets.mk
