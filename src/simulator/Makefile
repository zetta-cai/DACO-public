include ../../common/mk/recipes.mk

DEPS :=
CLEAN :=

##############################################################################
# boost module

BOOST_LDDIR := -L../../lib/boost_1_81_0/install/lib
LDDIR += $(BOOST_LDDIR)
BOOST_LDLIBS := -l:libboost_program_options.a -l:libboost_json.a -l:libboost_system.a -l:libboost_filesystem.a
LDLIBS += $(BOOST_LDLIBS)

BOOST_INCDIR := -I../../lib/boost_1_81_0/install/include
INCDIR += $(BOOST_INCDIR)

##############################################################################
# common module

COMMON_INCDIR := -I../../common
INCDIR += $(COMMON_INCDIR)

DEPS += ../../common/config.d ../../common/util.d ../../common/param.d
CLEAN += ../../common/config.o ../../common/util.o ../../common/param.o
COMMON_OBJECTS += ../../common/config.o ../../common/util.o ../../common/param.o
COMMON_SHARED_OBJECTS += ../../common/config.shared.o ../../common/util.shared.o ../../common/param.shared.o

##############################################################################
# Executable files

main: main.o $(COMMON_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += main.d
CLEAN += main.o

##############################################################################

TARGETS := main

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
