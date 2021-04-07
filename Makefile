# General

PKG_CONFIG ?= pkg-config
FIND ?= find
CC ?= gcc
CXX ?= g++
TARGET_GCC ?= gcc
RMRF ?= rm -rf
MKDIRP ?= mkdir -p

GCCPLUGINS_DIR := $(shell $(TARGET_GCC) -print-file-name=plugin)

DEPSDIR ?= $(CURDIR)/deps
BUILDDIR := $(CURDIR)/build
BUILDDIR_TESTS := $(BUILDDIR)/tests_objs
BUILDDIR_PLUGIN := $(BUILDDIR)/plugin_objs
BUILDDIR_DEPS := $(BUILDDIR)/deps
DESTDIR ?= $(GCCPLUGINS_DIR)

SOURCES := $(shell $(FIND) src -name '*.c' -or -name '*.cxx')

CPPFLAGS += -I$(GCCPLUGINS_DIR)/include \
            -I$(CURDIR)/include \
            -I$(CURDIR)/tests

COMMON_FLAGS := -fshort-enums \
                -funsafe-loop-optimizations \
                -fstrict-aliasing \
                -fipa-pure-const \
                -fpic

# Warnings
COMMON_FLAGS += -Wall \
                -Wextra \
                -Wformat=2 \
                -Wformat-overflow=2 \
                -Wshift-overflow=2 \
                -Wduplicated-branches \
                -Wshadow \
                -Wduplicated-cond \
                -Wswitch-default \
                -Wmaybe-uninitialized \
                -Wunsafe-loop-optimizations \
                -Wcast-qual \
                -Wcast-align=strict \
                -Wconversion \
                -Wdangling-else \
                -Wlogical-op \
                -Wmissing-field-initializers \
                -Winline \
                -Wdisabled-optimization \
                -Wstrict-aliasing=3 \
                -Wsuggest-attribute=const \
                -Wsuggest-attribute=noreturn \
                -Wsuggest-attribute=malloc \
                -Wsuggest-attribute=format \
                -fanalyzer \
                -fanalyzer-transitivity \
                -fanalyzer-checker=taint

ifeq ($(findstring 1,$(NDEBUG)),1)
    COMMON_FLAGS += -O3 -flto
else
# TODO Address sanitizer.
# Error: "Interceptors are not working.
# This may be because AddressSanitizer is loaded too late (e.g. via dlopen). "
    COMMON_FLAGS += -O0 -g \
					-fsanitize=leak \
					-fsanitize=undefined \
					-fsanitize=shift \
					-fsanitize=bounds \
					-fsanitize-address-use-after-scope \
					-fno-omit-frame-pointer
endif


## GMP is required by gcc.
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags-only-I gmp)

## Jansson for manipulating compile_commands.json
CPPFLAGS += $(shell $(PKG_CONFIG) --cflags-only-I jansson)
COMMON_FLAGS += $(shell $(PKG_CONFIG) --cflags-only-other jansson)
LDFLAGS += $(shell $(PKG_CONFIG) --libs jansson)

## Same flags for C and C++.
CFLAGS += $(COMMON_FLAGS) \
          -std=gnu18 \
          -Wstrict-prototypes \
          -Wmissing-prototypes

CXXFLAGS += $(COMMON_FLAGS) -std=c++17

# Plugin specific

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    PLUGIN_CFLAGS += -dynamiclib -undefined dynamic_lookup
    PLUGIN_CXXFLAGS += -dynamiclib -undefined dynamic_lookup
else
    PLUGIN_CFLAGS += -shared
    PLUGIN_CXXFLAGS += -shared
endif

PLUGIN_FILENAME ?= gen_ccommands.so

# Specific for UnitTesting.

## CppUTest
### Memory leak detection doesn't play well with GCC headers.
CPPUTEST_CONFIGURE_FLAGS := --disable-memory-leak-detection
CPPUTEST_CPPFLAGS := -DCPPUTEST_MEM_LEAK_DETECTION_DISABLED
TESTS_LDFLAGS += -L$(BUILDDIR_DEPS)/cpputest/lib -lstdc++ -lCppUTest -lCppUTestExt
TESTS_CPPFLAGS += -I$(DEPSDIR)/cpputest/include $(CPPUTEST_CPPFLAGS)

TESTS_SOURCES := $(shell $(FIND) tests -name '*.c' -or -name '*.cxx')

# Gen CCommands
CCMD_PLUGIN_PATH ?= $(GCCPLUGINS_DIR)/$(PLUGIN_FILENAME)
ifeq ($(findstring $(CCMD_PLUGIN_PATH),$(wildcard $(CCMD_PLUGIN_PATH))), \
      $(CCMD_PLUGIN_PATH))
    GEN_CCMDS ?= 1
endif

$(error, $(PLUGIN_FILENAME))
ifeq ($(findstring 1,$(GEN_CCMDS)), 1)
    CCMD_FLAGS = -fplugin=$(CCMD_PLUGIN_PATH) \
                  -fplugin-arg-gen_ccommands-filter_specific \
                  -fplugin-arg-gen_ccommands-filter_internal \
                  -fplugin-arg-gen_ccommands-output=$(@:.o=.json)
    CCMD_FLAGS_CC := -fplugin-arg-gen_ccommands-replace_comp="gcc"
    CCMD_FLAGS_CXX := -fplugin-arg-gen_ccommands-replace_comp="g++"
endif

# Obj files

PLUGIN_OBJS := $(addprefix $(BUILDDIR_PLUGIN)/, $(addsuffix .o,$(basename $(SOURCES))))
TESTS_OBJS := $(addprefix $(BUILDDIR_TESTS)/, $(addsuffix .o,$(basename $(TESTS_SOURCES) $(SOURCES))))

# Rules
.PHONY: install_dirs build_dirs all tests plugin deps

all: plugin tests

plugin: $(BUILDDIR)/$(PLUGIN_FILENAME)

tests: $(BUILDDIR)/tests

install: $(BUILDDIR)/$(PLUGIN_FILENAME)
	@$(MKDIRP) $(DESTDIR)
	@cp $< $(DESTDIR)/$(PLUGIN_FILENAME)

clean:
	@$(RMRF) $(BUILDDIR)

deps: $(BUILDDIR_DEPS)/cpputest

$(BUILDDIR)/$(PLUGIN_FILENAME): $(PLUGIN_OBJS)
	@$(MKDIRP) $(dir $@)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(PLUGIN_CPPFLAGS) $(PLUGIN_CXXFLAGS) $(PLUGIN_LDFLAGS) $^ -o $@

$(BUILDDIR_PLUGIN)/%.o: %.c
	@$(MKDIRP) $(dir $@)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $(PLUGIN_CPPFLAGS) $(PLUGIN_CFLAGS) $< -o $@ \
			$(CCMD_FLAGS) \
			$(CCMD_FLAGS_CC)

$(BUILDDIR_PLUGIN)/%.o: %.cxx
	@$(MKDIRP) $(dir $@)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(PLUGIN_CPPFLAGS) $(PLUGIN_CXXFLAGS) $< -o $@ \
			$(CCMD_FLAGS) \
			$(CCMD_FLAGS_CXX)

$(BUILDDIR)/tests: $(TESTS_OBJS) $(BUILDDIR_DEPS)/cpputest
	@$(MKDIRP) $(dir $@)
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(TESTS_CPPFLAGS) $(TESTS_CXXFLAGS) $(TESTS_LDFLAGS) $(TESTS_OBJS) -o $@

$(BUILDDIR_DEPS)/cpputest:
	@$(MKDIRP) $(BUILDDIR_DEPS)/cpputest
	@autoreconf -i $(DEPSDIR)/cpputest
	@cd $(BUILDDIR_DEPS)/cpputest && \
		env CPPFLAGS=$(CPPUTEST_CPPFLAGS) $(DEPSDIR)/cpputest/configure $(CPPUTEST_CONFIGURE_FLAGS)
	@$(MAKE) -C $(BUILDDIR_DEPS)/cpputest

$(BUILDDIR_TESTS)/%.o: %.c
	@$(MKDIRP) $(dir $@)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $(TESTS_CPPFLAGS) $(TESTS_CFLAGS) $< -o $@ \
			$(CCMD_FLAGS) \
			$(CCMD_FLAGS_CC)

$(BUILDDIR_TESTS)/%.o: %.cxx
	@$(MKDIRP) $(dir $@)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(TESTS_CPPFLAGS) $(TESTS_CXXFLAGS) $< -o $@ \
			$(CCMD_FLAGS) \
			$(CCMD_FLAGS_CXX)

# Dependency files

$(BUILDDIR_TESTS)/%.d: %.c
	@$(MKDIRP) $(dir $@)
	@$(CC) $(CPPFLAGS) $(TESTS_CPPFLAGS) -M -MG -MT $(@:.d=.o) -MF $@ $< -o /dev/null

$(BUILDDIR_TESTS)/%.d: %.cxx
	@$(MKDIRP) $(dir $@)
	@$(CXX) $(CPPFLAGS) $(TESTS_CPPFLAGS) -M -MG -MT $(@:.d=.o) -MF $@ $< -o /dev/null

$(BUILDDIR_PLUGIN)/%.d: %.c
	@$(MKDIRP) $(dir $@)
	@$(CC) $(CPPFLAGS) $(PLUGIN_CPPFLAGS) -M -MG -MT $(@:.d=.o) -MF $@ $< -o /dev/null

$(BUILDDIR_PLUGIN)/%.d: %.cxx
	@$(MKDIRP) $(dir $@)
	@$(CXX) $(CPPFLAGS) $(PLUGIN_CPPFLAGS) -M -MG -MT $(@:.d=.o) -MF $@ $< -o /dev/null

include $(PLUGIN_OBJS:.o=.d)
include $(TESTS_OBJS:.o=.d)
