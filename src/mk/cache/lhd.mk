# lhd module

LHD_DIRPATH = $(LIBRARY_DIRPATH)/lhd
LHD_INCDIR = -I$(LHD_DIRPATH)
INCDIR += $(LHD_INCDIR)

# NOT used in hacked version
#LHD_SRCFILES = $(LHD_DIRPATH)/repl.cpp

# hacked part
LHD_SRCFILES += $(COVERED_DIRPATH)/cache/lhd/lhd.cpp

LHD_OBJECTS += $(LHD_SRCFILES:.cpp=.opp)
LHD_SHARED_OBJECTS += $(LHD_SRCFILES:.cpp=.shared.opp)
DEPS += $(LHD_SRCFILES:.cpp=.dpp)
CLEANS += $(LHD_OBJECTS) $(LHD_SHARED_OBJECTS)
LINK_OBJECTS += $(LHD_OBJECTS)