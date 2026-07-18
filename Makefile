# ==== Toolchain / Target ====
TRIPLE  ?= aarch64-linux-gnu
CC       = $(TRIPLE)-gcc
AR       = $(TRIPLE)-ar

# ==== Sysroot ====
HOST_ARCH := $(shell uname -m)
ifneq ($(MAKECMDGOALS),clean)
ifeq ($(strip $(SYSROOT)),)
  ifneq ($(HOST_ARCH),aarch64)
    $(error "Error: SYSROOT is not defined and host architecture is $(HOST_ARCH). Cross-compilation requires SYSROOT to be set")
  endif
endif
endif
ifneq ($(strip $(SYSROOT)),)
  SYSROOT_CFLAGS  := --sysroot=$(SYSROOT)
  SYSROOT_LDFLAGS := --sysroot=$(SYSROOT)
endif

# ==== Flags ====
CFLAGS  := -Wall -Wextra -O3 -fPIC -Iinclude $(SYSROOT_CFLAGS)
LDFLAGS := -shared $(SYSROOT_LDFLAGS)

# ==== Debug logging (build with HOV_DEBUG=1 to enable) ====
ifdef HOV_DEBUG
  CFLAGS += -DHOV_DEBUG
endif

# ==== Annotate ====
ANNOTATE_DIR ?= $(CURDIR)/../annotate
export ANNOTATE_DIR

# ==== Util ====
UTIL_DIR ?= $(CURDIR)/tests/kernel/util
export UTIL_DIR

# ==== Directories ====
SRC_DIR := src
INC_DIR := include
LIB_DIR := lib

# ==== Core library sources (linked into workloads) ====
LIB_SRC_C := $(SRC_DIR)/hov.c $(SRC_DIR)/hov_pool.c $(SRC_DIR)/hov_alloc.c
LIB_SRC_S := $(SRC_DIR)/hov_asm.S
LIB_OBJ   := $(LIB_SRC_C:.c=.o) $(LIB_SRC_S:.S=.o)

LIB_STATIC := $(LIB_DIR)/libhov.a
LIB_SHARED := $(LIB_DIR)/libhov.so

# ==== Syscall interposer (standalone LD_PRELOAD library) ====
SYSCALLS_SRC := $(SRC_DIR)/hov_syscalls.c
SYSCALLS_OBJ := $(SYSCALLS_SRC:.c=.o)
SYSCALLS_SO  := $(LIB_DIR)/libhov_syscalls.so

# ==== Test directories ====
UNIT_DIRS   := $(sort $(dir $(wildcard tests/unit/*/Makefile)))
KERNEL_DIRS := $(sort $(dir $(wildcard tests/kernel/*/Makefile)))

# ============================================================================
# Targets
# ============================================================================

.PHONY: all lib unit kernels clean

all: lib unit

# NOTE: Build kernels separately with 'make kernels'.
# Requires aarch64-linux-gnu-g++ which may not be installed.

# ---- Library ----
lib: $(LIB_STATIC) $(LIB_SHARED) $(SYSCALLS_SO)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.S
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_STATIC): $(LIB_OBJ) | $(LIB_DIR)/.
	$(AR) rcs $@ $^

$(LIB_SHARED): $(LIB_OBJ) | $(LIB_DIR)/.
	$(CC) $(LDFLAGS) -o $@ $^

$(SYSCALLS_SO): $(SYSCALLS_OBJ) | $(LIB_DIR)/.
	$(CC) -shared $(SYSROOT_LDFLAGS) -o $@ $^ -ldl

$(LIB_DIR)/.:
	mkdir -p $(LIB_DIR)

# ---- Unit tests ----
unit: lib
	@for dir in $(UNIT_DIRS); do \
		echo "==== Building unit test: $$dir ===="; \
		$(MAKE) -C $$dir CC=$(CC) SYSROOT=$(SYSROOT) || exit 1; \
	done

# ---- Kernels ----
kernels:
	@for dir in $(KERNEL_DIRS); do \
		echo "==== Building kernel: $$dir ===="; \
		$(MAKE) -C $$dir all || exit 1; \
	done

# ---- Clean ----
clean:
	rm -f $(LIB_OBJ) $(SYSCALLS_OBJ) $(LIB_STATIC) $(LIB_SHARED) $(SYSCALLS_SO)
	@for dir in $(UNIT_DIRS) $(KERNEL_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
