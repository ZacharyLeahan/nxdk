XBE_TITLE = nxdk\ pbkit\ queue\ $(VARIANT)\ $(MODE)
SRCS = $(CURDIR)/main.c $(CURDIR)/pbkit_trace_core.c
NXDK_DIR ?= $(CURDIR)/../..

VARIANT ?= patched
MODE ?= 480p
REFRESHES ?= 3600

TEST_CFLAGS = -DPBKIT_QUEUE_TEST_REFRESHES=$(REFRESHES)
TEST_CFLAGS += -DPBKIT_QUEUE_VARIANT=\"$(VARIANT)\"

ifeq ($(VARIANT),stock)
TEST_CFLAGS += -DPBKIT_QUEUE_TEST_STOCK
else ifneq ($(VARIANT),patched)
$(error VARIANT must be stock or patched)
endif

ifeq ($(MODE),720p)
TEST_CFLAGS += -DPBKIT_QUEUE_TEST_720P
else ifneq ($(MODE),480p)
$(error MODE must be 480p or 720p)
endif

$(CURDIR)/main.obj $(CURDIR)/pbkit_trace_core.obj: CFLAGS += $(TEST_CFLAGS)

include $(NXDK_DIR)/Makefile
