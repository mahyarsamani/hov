# ==== Toolchain / Target ====
TRIPLE ?= aarch64-linux-gnu
CXX     = $(TRIPLE)-g++
CC      = $(TRIPLE)-gcc

ifdef CLANG
TARGET_FLAG := --target=$(TRIPLE)
endif

# ==== Sysroot ====
HOST_ARCH := $(shell uname -m)
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(HOST_ARCH),aarch64)
ifndef SYSROOT
$(error SYSROOT must be set when cross-compiling on $(HOST_ARCH))
endif
endif
endif
ifneq ($(strip $(SYSROOT)),)
  SYSROOT_CFLAGS  := --sysroot=$(SYSROOT)
  SYSROOT_LDFLAGS := --sysroot=$(SYSROOT) \
                     -Wl,-rpath-link,$(SYSROOT)/lib \
                     -Wl,-rpath-link,$(SYSROOT)/lib/$(TRIPLE) \
                     -Wl,-rpath-link,$(SYSROOT)/usr/lib \
                     -Wl,-rpath-link,$(SYSROOT)/usr/lib/$(TRIPLE) \
                     -Wl,-rpath-link,$(SYSROOT)/lib/aarch64-linux-gnu \
                     -Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-gnu
endif

# ==== Paths (relative to workloads/hov/) ====
HOV_ROOT := ../../..
HOV_INC  := $(HOV_ROOT)/include
HOV_LIB  := $(HOV_ROOT)/lib

# ==== Optimization & ISA flags ====
OPTFLAGS   := -O3
ifneq (,$(findstring aarch64,$(TRIPLE)))
  SVE_FLAGS := -DUSE_SVE -march=armv8.6-a+sve -msve-vector-bits=512
endif

# ==== gem5 annotations & util paths ====
ifndef ANNOTATE_DIR
  $(error ANNOTATE_DIR is not defined. Please run from the top level or define it.)
endif
ifeq ($(wildcard $(ANNOTATE_DIR)),)
  $(error ANNOTATE_DIR $(ANNOTATE_DIR) does not exist.)
endif

ifndef UTIL_DIR
  $(error UTIL_DIR is not defined. Please run from the top level or define it.)
endif
ifeq ($(wildcard $(UTIL_DIR)),)
  $(error UTIL_DIR $(UTIL_DIR) does not exist.)
endif

ANNOTATE_FLAGS := -DANNOTATE
ANNOTATE_INC   := -I$(ANNOTATE_DIR)/include
ANNOTATE_LIB   := -L$(ANNOTATE_DIR)/lib
ANNOTATE_LDLIBS := -lannotate.gem5se -lm5
UTIL_INC       := -I$(UTIL_DIR)

# ==== Sources / Outputs ====
SRC         := dot_indirect.cc
BINARY_BASE := dot_indirect

# ==== Common flags ====
CXXWARN   := -Wall -Wextra -Wno-unused-parameter
CXXSTD    := -std=c++17
CXXBASE   := $(TARGET_FLAG) $(OPTFLAGS) $(CXXSTD) $(CXXWARN) $(SYSROOT_CFLAGS) \
             $(ANNOTATE_FLAGS) $(ANNOTATE_INC) $(UTIL_INC)
LDFLAGS   := $(SYSROOT_LDFLAGS) $(ANNOTATE_LIB) $(ANNOTATE_LDLIBS)

# ==== Helpers ====
define build_bin
$(BINARY_BASE)$1: $(SRC)
	$(CXX) $(CXXBASE) $(2) -o $$@ $$< $(LDFLAGS) $(3)
endef

# ============================================================================
# Targets
# ============================================================================

.PHONY: all no-opt sve hov clean print-config

all: no-opt sve hov

print-config:
	@echo "CXX            = $(CXX)"
	@echo "TRIPLE         = $(TRIPLE)"
	@echo "SYSROOT        = $(SYSROOT)"
	@echo "CXXBASE        = $(CXXBASE)"
	@echo "LDFLAGS        = $(LDFLAGS)"
	@echo "SVE_FLAGS      = $(SVE_FLAGS)"

# ---- no-opt ----
$(eval $(call build_bin,,$(EMPTY),$(EMPTY)))

no-opt: $(BINARY_BASE)

# ---- SVE ----
ifneq ($(strip $(SVE_FLAGS)),)
  $(eval $(call build_bin,_sve,$(SVE_FLAGS),$(EMPTY)))
endif

sve: $(if $(SVE_FLAGS),$(BINARY_BASE)_sve,)
	$(if $(SVE_FLAGS),,@echo "SVE not available for TRIPLE=$(TRIPLE)")

# ---- HOV ----
HOV_FLAGS  := -DUSE_HOV -I$(HOV_INC)
HOV_LDLIBS := $(HOV_LIB)/libhov.a

$(eval $(call build_bin,_hov,$(HOV_FLAGS),$(HOV_LDLIBS)))

hov: $(BINARY_BASE)_hov

# ---- clean ----
clean:
	rm -f $(BINARY_BASE) $(BINARY_BASE)_sve $(BINARY_BASE)_hov
