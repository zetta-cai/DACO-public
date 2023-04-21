# Basic recipes
include src/mk/recipes.mk

# Third-party recipes
include src/mk/boost.mk
include src/mk/cachebench.mk

# COVERED's recipes
include src/mk/covered.mk

##############################################################################
# Executable files

simulator: src/simulator.o $(COMMON_OBJECTS) $(BENCHMARK_OBJECTS) $(WORKLOAD_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += src/simulator.d
CLEAN += src/simulator.o

##############################################################################

TARGETS := simulator

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
