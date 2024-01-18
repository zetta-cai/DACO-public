# frozenhot module

FROZENHOT_DIRPATH = $(LIBRARY_DIRPATH)/frozenhot
# -I$(FROZENHOT_DIRPATH)/CLHT/external/include
FROZENHOT_INCDIR = -I$(FROZENHOT_DIRPATH)/CLHT/include -I$(FROZENHOT_DIRPATH)
INCDIR += $(FROZENHOT_INCDIR)

# NOTE: FrozenHot uses libtbb for unordered map of dynamic cache and libclht for fast hash table of frozen cache
FROZENHOT_LDLIBS := -l:libtbb.so -l:libclht.a
LDLIBS += $(FROZENHOT_LDLIBS)

FROZENHOT_LDDIR = -L$(FROZENHOT_DIRPATH)/CLHT
LDDIR += $(FROZENHOT_LDDIR)

# hacked part
# NOTE: FrozenHot uses template class -> NO need to compile into binary objects and static/dynamic libraries