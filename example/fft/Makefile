################################################################################
#   Makefile for test of fft on tms320c55x
################################################################################
DEVREV = 5509:E
TARGET = test.out
#
TMSLIBDIR ?= ../../
#
OBJDIR = obj
LISTDIR = list
SRCDIR = src
INCLUDEDIR = include
#
SRCS = test.c
#
OBJS_FULL = $(patsubst %,$(OBJDIR)/%,$(filter %.o,$(OBJS)))
CFLAGS += -I$(TMSLIBDIR)/include
LDFLAGS += -i$(TMSLIBDIR)
#
all: all_rules

include $(TMSLIBDIR)/Makefile.rules

$(TARGET): $(OBJS)
	$(LNK) $(LDFLAGS) -m fft_test.map -o $@ $(OBJS_FULL) \
		-ldsp55x.lib $(RTSLIB) $(TMSLIBDIR)/tms320c55x.cmd

clean: clean_rules

# Naming our phony targets
.PHONY: all clean

