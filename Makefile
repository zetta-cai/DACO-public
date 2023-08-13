# Basic recipes
include src/mk/recipes.mk

# Recipes for third-party lib
include src/mk/lib/boost.mk
include src/mk/lib/cachebench.mk
include src/mk/lib/rocksdb.mk
include src/mk/lib/smhasher.mk

# Recipes for cache methods (including baselines and COVERED)
include src/mk/cache/cachelib.mk
include src/mk/cache/covered.mk
include src/mk/cache/lfu.mk
include src/mk/cache/lru.mk
include src/mk/cache/segcache.mk

##############################################################################
# Set link directory paths for standard libraries

LDDIR += -L/usr/lib/x86_64-linux-gnu

##############################################################################
# Executable files

simulator: src/simulator.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/simulator.d
CLEANS += src/simulator.o

total_statistics_loader: src/total_statistics_loader.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/total_statistics_loader.d
CLEANS += src/total_statistics_loader.o

dataset_loader: src/dataset_loader.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/dataset_loader.d
CLEANS += src/dataset_loader.o

#statistics_aggregator: src/statistics_aggregator.o $(LINK_OBJECTS)
#	$(LINK) $^ $(LDLIBS) -o $@
#DEPS += src/statistics_aggregator.d
#CLEANS += src/statistics_aggregator.o

##############################################################################

# statistics_aggregator
TARGETS := simulator total_statistics_loader dataset_loader

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
