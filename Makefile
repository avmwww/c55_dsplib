################################################################################
#   Makefile for DSP 55x lib                                                   #
################################################################################
DEVREV = 5509:E
TARGET = dsp55x.lib
#
#
OBJDIR = obj
LISTDIR = list
SRCDIR = src
INCLUDEDIR = include
#*******************************************************************************
SRCS =  fft.a55 \
	gost.a55 \
	gost.c \

#*******************************************************************************
OUTS = $(patsubst %,$(OBJDIR)/%,$(filter %.o,$(OBJS)))
#
all: all_rules

include Makefile.rules

$(TARGET): $(OBJS)
	$(AR) -r $(TARGET) $(OUTS)

clean: clean_rules

# Naming our phony targets
.PHONY: all clean


