##############################################################################
# Global variables

# NOTE: used by exp scripts to parse library path -> "JSON value: " MUST be the same as scripts/tools/parse_config.py
LIBRARY_DIRPATH = $(shell python3 -m scripts.tools.parse_config library_dirpath | tail -n 1 | sed -n 's/^JSON value: \(.*\)/\1/p')

##############################################################################
# Include other Makefiles

# Basic recipes
include src/mk/recipes.mk

# Recipes for cache methods (including baselines and COVERED)
include src/mk/cache/arc.mk
include src/mk/cache/cachelib.mk
include src/mk/cache/covered.mk
include src/mk/cache/fifo.mk
include src/mk/cache/frozenhot.mk
include src/mk/cache/glcache.mk
include src/mk/cache/gdsf.mk
include src/mk/cache/lfu.mk
include src/mk/cache/lhd.mk
include src/mk/cache/lrb.mk
include src/mk/cache/lru.mk
include src/mk/cache/s3fifo.mk
include src/mk/cache/segcache.mk
include src/mk/cache/sieve.mk
include src/mk/cache/slru.mk

# Recipes for third-party lib
include src/mk/lib/boost.mk
include src/mk/lib/cachebench.mk
include src/mk/lib/rocksdb.mk
include src/mk/lib/smhasher.mk
include src/mk/lib/tommyds.mk

##############################################################################
# Set link directory paths for standard libraries

LDDIR += -L/usr/lib/x86_64-linux-gnu

##############################################################################
# Executable files

dataset_loader: src/dataset_loader.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/dataset_loader.d
CLEANS += src/dataset_loader.o

simulator: src/simulator.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/simulator.d
CLEANS += src/simulator.o

client: src/client.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/client.d
CLEANS += src/client.o

edge: src/edge.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/edge.d
CLEANS += src/edge.o

cloud: src/cloud.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/cloud.d
CLEANS += src/cloud.o

evaluator: src/evaluator.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/evaluator.d
CLEANS += src/evaluator.o

cliutil: src/cliutil.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/cliutil.d
CLEANS += src/cliutil.o

total_statistics_loader: src/total_statistics_loader.o $(LINK_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/total_statistics_loader.d
CLEANS += src/total_statistics_loader.o

#statistics_aggregator: src/statistics_aggregator.o $(LINK_OBJECTS)
#	$(LINK) $^ $(LDLIBS) -o $@
#DEPS += src/statistics_aggregator.d
#CLEANS += src/statistics_aggregator.o

##############################################################################

# statistics_aggregator
TARGETS := dataset_loader simulator client edge cloud evaluator cliutil total_statistics_loader

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
