include mk/recipes.mk
include mk/boost.mk

##############################################################################
# common module

DEPS += common/config.d common/util.d common/param.d
CLEAN += common/config.o common/util.o common/param.o
COMMON_OBJECTS += common/config.o common/util.o common/param.o
COMMON_SHARED_OBJECTS += common/config.shared.o common/util.shared.o common/param.shared.o

##############################################################################
# Executable files

simulator: simulator.o $(COMMON_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += simulator.d
CLEAN += simulator.o

##############################################################################

TARGETS := simulator

all: $(TARGETS)
#	rm -rf $(CLEAN) $(DEPS)

##############################################################################

clean:
	rm -rf $(CLEAN) $(DEPS) $(TARGETS)

.DEFAULT_GOAL := all
.PHONY: all clean

# Include dependencies
ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif
