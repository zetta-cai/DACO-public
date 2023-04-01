# Generic recipes

##############################################################################
# Generate dependency files

DEPFLAGS ?= -MT $(@:.d=.o) -MMD -MP -MF $(@:.d=.Td)
POSTCOMPILE_DEPS = mv -f $(@:.d=.Td) $@
%.d: %.c
	$(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $^
	@$(POSTCOMPILE_DEPS)
.PRECIOUS: %.d

##############################################################################
# Preprocessing, compilation, and link macros

INCDIR += -I/usr/include -I./lib/boost/include
CPPFLAGS += $(INCDIR)
CPPFLAGS += $(EXTRA_CPPFLAGS)

CC := g++
#CFLAGS += -std=c++14 -O3 -g -Wall -Werror -march=native -fno-omit-frame-pointer
CFLAGS += -std=c++14 -O3 -g -Wall -march=native -fno-omit-frame-pointer
CFLAGS += $(EXTRA_CFLAGS)
CFLAGS_SHARED += $(CFLAGS) -fPIC

LDDIR := -L./lib/boost/lib
LDLIBS := -lboost
LDFLAGS += $(LDDIR)
LINK = $(CC) $(LDFLAGS)
LINK.so = $(CC) $(LDFLAGS) -shared

##############################################################################
# Compilation commands

# Compile C to object file while generating dependency
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) -c
OUTPUT_OPTION.c ?= -o $@
%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION.c) $<

# Compile C to position independent object file while generating dependency
COMPILE_SHARED.c = $(CC) $(CFLAGS_SHARED) $(CPPFLAGS) -c
%.shared.o: %.c
	$(COMPILE_SHARED.c) $(OUTPUT_OPTION.c) $<

##############################################################################
# Link and archive examples

# Link binary from objects
#%: %.o
#	$(LINK) $^ $(LDLIBS) -o $@

# Link shared library from objects
#%.so:
#	$(LINK.so) $^ $(LDLIBS) $(OUTPUT_OPTION.c)

# Archive static library from objects
#libcommon.a: $(UTILS_OBJECTS) $(ROCKSDB_OBJECTS)
#	ar -rcs $@ $^

# Archive position independent static library from objects
#libcommon_shared.a: $(UTILS_SHARED_OBJECTS) $(ROCKSDB_SHARED_OBJECTS)
#	ar -rcs $@ $^

##############################################################################