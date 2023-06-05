# Basic recipes
include src/mk/recipes.mk

# Recipes for third-party lib
include src/mk/lib/boost.mk
include src/mk/lib/cachebench.mk
include src/mk/lib/rocksdb.mk
include src/mk/lib/smhasher.mk

# Recipes for cache methods (including baselines and COVERED)
include src/mk/cache/covered.mk
include src/mk/cache/lru.mk

##############################################################################
# Set link objects and directory paths

LINK_OBJECTS = $(COVERED_OBJECTS) $(CACHEBENCH_OBJECTS) $(LRU_OBJECTS) $(SMHASHER_OBJECTS)

LDDIR += $(BOOST_LDDIR)
LDDIR += $(CACHEBENCH_LDDIR)
LDDIR += $(ROCKSDB_LDDIR)
LDDIR += -L/usr/lib/x86_64-linux-gnu

##############################################################################
# Executable files

simulator: src/simulator.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/simulator.d
CLEANS += src/simulator.o

statistics_aggregator: src/statistics_aggregator.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/statistics_aggregator.d
CLEANS += src/statistics_aggregator.o

##############################################################################

TARGETS := simulator statistics_aggregator

all: $(TARGETS)
#	rm -rf $(CLEANS) $(DEPS)

##############################################################################

clean:
	rm -rf $(CLEANS) $(DEPS) $(TARGETS)

.DEFAULT_GOAL := all
.PHONY: all clean

# Include dependencies
ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif
