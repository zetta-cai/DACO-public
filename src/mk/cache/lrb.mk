# lrb module

LRB_DIRPATH = $(LIBRARY_DIRPATH)/lrb
LRB_INCDIR = -I$(LRB_DIRPATH)/lib/sparsepp -I$(LRB_DIRPATH)/install/lightgbm/include -I$(LRB_DIRPATH)/install/mongocxxdriver/include/mongocxx/v_noabi -I$(LRB_DIRPATH)/install/mongocxxdriver/include/bsoncxx/v_noabi -I$(LRB_DIRPATH)/install/webcachesim/include
INCDIR += $(LRB_INCDIR)

# -l:libbson-1.0.so -l:libmongoc-1.0.so
LRB_LDLIBS := -l:lib_lightgbm.so -l:libmongocxx.so -l:libbsoncxx.so -l:libwebcachesim.so
LDLIBS += $(LRB_LDLIBS)

# -L$(LRB_DIRPATH)/install/mongocdriver/lib
LRB_LDDIR = -L$(LRB_DIRPATH)/install/lightgbm/lib -L$(LRB_DIRPATH)/install/mongocxxdriver/lib -L$(LRB_DIRPATH)/install/webcachesim/lib
LDDIR += $(LRB_LDDIR)

# hacked part
LRB_SRCFILES = $(wildcard $(COVERED_DIRPATH)/cache/lrb/*.cpp)
LRB_OBJECTS += $(LRB_SRCFILES:.cpp=.opp)
LRB_SHARED_OBJECTS += $(LRB_SRCFILES:.cpp=.shared.opp)
DEPS += $(LRB_SRCFILES:.cpp=.dpp)
CLEANS += $(LRB_OBJECTS) $(LRB_SHARED_OBJECTS)
LINK_OBJECTS += $(LRB_OBJECTS)