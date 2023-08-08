# cachelib module

# NOTE: include path and lib path of Cachelib have been added by src/mk/lib/cachebench.mk

# hacked part (CACHELIB uses template class -> NO need to compile into binary objects and static/dynamic libraries)
#CACHELIB_SRCFILES = $(wildcard $(COVERED_DIRPATH)/cache/cachelib/*.c)
#CACHELIB_OBJECTS += $(CACHELIB_SRCFILES:.c=.o)
#CACHELIB_SHARED_OBJECTS += $(CACHELIB_SRCFILES:.c=.shared.o)
#DEPS += $(CACHELIB_SRCFILES:.c=.d)
#CLEANS += $(CACHELIB_OBJECTS) $(CACHELIB_SHARED_OBJECTS)