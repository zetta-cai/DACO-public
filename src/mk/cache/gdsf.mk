# gdsf module

# NOTE: NOT used by src/cache/greedydual/*
#GDSF_DIRPATH = lib/webcachesim
#GDSF_INCDIR = $(GDSF_DIRPATH)/caches
#INCDIR += $(GDSF_INCDIR)

# hacked part
GDSF_SRCFILES = $(wildcard $(COVERED_DIRPATH)/cache/greedydual/*.c)
GDSF_OBJECTS += $(GDSF_SRCFILES:.c=.o)
GDSF_SHARED_OBJECTS += $(GDSF_SRCFILES:.c=.shared.o)
DEPS += $(GDSF_SRCFILES:.c=.d)
CLEANS += $(GDSF_OBJECTS) $(GDSF_SHARED_OBJECTS)
LINK_OBJECTS += $(GDSF_OBJECTS)