# Generic recipes

# NOTE: appending (+=) will inherit simple assignment (:=) or recursive assignment (=)
# Example: for appending with simple assignment, A += B <-> A := A B
# Example: for appending with recursive assignment, A += B <-> A = A B
DEPS =
CLEANS =

##############################################################################
# Generate dependency files

# Note: = (always replace), ?= (replace if undefined before), and += (append) are on-demand/delayed expanding; while := (always replace) is immediate expanding (usually used for constant right value)
#DEPFLAGS ?= -MT $(@:.d=.o) -MMD -MP -MF $(@:.d=.Td)
DEPENDENCY.c ?= $(CC) $(CFLAGS) $(CPPFLAGS) -MM
# Note: $< refers to the first input file (i.e., $*.c), while $^ refers to all input files (including many .h files combined from .d files)
# Note: \1 refers to the string matched by \($*\), i.e., the target name ($*) in Makefile
# Note: $$ refers to the dollar sign itself in Makefile
%.d: %.c
	$(DEPENDENCY.c) $*.c > $*.Td || rm $*.Td
	@sed 's,\($(notdir $*)\)\.o[ :]*,$*.o $@ : ,g' $*.Td > $@
#	@sed -e 's/.*://' -e 's/\\$$//' < $*.Td | fmt -1 | \
#	  sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.Td
.PRECIOUS: %.d

##############################################################################
# Preprocessing, compilation, and link macros

INCDIR += -I/usr/include
CPPFLAGS += $(INCDIR)
CPPFLAGS += $(EXTRA_CPPFLAGS)

#CC := g++
# For compile debugging (excluding -v)
CC := g++ -fsanitize=address

#CFLAGS += -std=c++17 -O3 -g -Wall -Werror -march=native -fno-omit-frame-pointer
#CFLAGS += -std=c++17 -O3 -g -Wall -march=native -fno-omit-frame-pointer
# Use -mtune=generic -march=x86-64 for level SSE2 of SIMD, such that folly will use F14IntrinsicsMode::Simd, which is the same as the definition compiled by cachelib.
CFLAGS += -std=c++17 -O3 -g -Wall -mtune=generic -march=x86-64 -fno-omit-frame-pointer
CFLAGS += $(EXTRA_CFLAGS)
CFLAGS_SHARED += $(CFLAGS) -fPIC

LDDIR =
LDLIBS =
# Uncomment for link debugging (excluding -v)
#LDFLAGS += -Wl,--trace
LDFLAGS += $(LDDIR)
LINK = $(CC) $(LDFLAGS)
LINK.so = $(CC) $(LDFLAGS) -shared

##############################################################################
# Compilation commands

# Compile C to object file while generating dependency
COMPILE.c = $(CC) $(CFLAGS) $(CPPFLAGS) -c
COMPILE_OUTPUT_OPTION.c ?= -o $@
%.o: %.c
	$(COMPILE.c) $(COMPILE_OUTPUT_OPTION.c) $<

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