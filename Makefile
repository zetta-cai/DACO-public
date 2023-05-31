# Basic recipes
include src/mk/recipes.mk

# Recipes for third-party lib
include src/mk/lib/boost.mk
include src/mk/lib/cachebench.mk
include src/mk/lib/rocksdb.mk

# Recipes for cache methods (including baselines and COVERED)
include src/mk/cache/covered.mk
include src/mk/cache/lru.mk

##############################################################################
# Executable files

simulator: src/simulator.o $(COVERED_OBJECTS) $(CACHEBENCH_OBJECTS) $(LRU_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/simulator.d
CLEANS += src/simulator.o

statistics_aggregator: src/statistics_aggregator.o $(COVERED_OBJECTS) $(CACHEBENCH_OBJECTS) $(LRU_OBJECTS)
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
