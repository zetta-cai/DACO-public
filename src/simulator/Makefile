include ../../common/mk/recipes.mk

DEPS :=
CLEAN :=

##############################################################################
# boost module

BOOST_LDDIR := -L../../lib/boost_1_81_0/install/lib
LDDIR += $(BOOST_LDDIR)
BOOST_LDLIBS := -l:libboost_program_options.a
LDLIBS += $(BOOST_LDLIBS)

BOOST_INCRDIR := -I../../lib/boost_1_81_0/install/include
INCDIR += $(BOOST_INCRDIR)

##############################################################################
# Utils module

UTILS_OBJECTS :=

#DEPS += key.d val.d key.shared.d val.shared.d
#CLEAN += key.o val.o key.shared.o val.shared.o
#UTILS_OBJECTS += key.o val.o
#UTILS_SHARED_OBJECTS += key.shared.o val.shared.o

##############################################################################
# Executable files

main: main.o $(UTILS_OBJECTS)
	$(LINK) $^ $(LDLIBS) -o $@
DEPS += main.d
CLEAN += main.o

##############################################################################

TARGETS := main

all: $(TARGETS)
	rm -rf $(CLEAN) $(DEPS)

##############################################################################

clean:
	rm -rf $(CLEAN) $(DEPS) $(TARGETS)

.DEFAULT_GOAL := all
.PHONY: all clean

# Include dependencies
ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif
