# Basic recipes
include src/mk/recipes.mk

# Third-party recipes
include src/mk/boost.mk
include src/mk/cachebench.mk
include src/mk/rocksdb.mk

# COVERED's recipes
include src/mk/covered.mk

##############################################################################
# Executable files

simulator: src/simulator.o $(COVERED_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/simulator.d
CLEAN += src/simulator.o

statistics_aggregator: src/statistics_aggregator.o $(COVERED_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/statistics_aggregator.d
CLEAN += src/statistics_aggregator.o

##############################################################################

TARGETS := simulator statistics_aggregator

all: $(TARGETS)
#	rm -rf $(CLEAN) $(DEPS)

##############################################################################

clean:
	rm -rf $(CLEAN) $(DEPS) $(TARGETS) $(CLEANS)

.DEFAULT_GOAL := all
.PHONY: all clean

# Include dependencies
ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif
