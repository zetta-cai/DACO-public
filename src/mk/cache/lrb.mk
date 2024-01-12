# lrb module

LRB_DIRPATH = $(LIBRARY_DIRPATH)/lrb
LRB_INCDIR = -I$(LRB_DIRPATH)/lib/sparsepp -I$(LRB_DIRPATH)/install/lightgbm/include -I$(LRB_DIRPATH)/install/mongocxxdriver/include -I$(LRB_DIRPATH)/install/webcachesim/include
INCDIR += $(LRB_INCDIR)

# -l:libbson-1.0.so -l:libmongoc-1.0.so -l:libbsoncxx.so
LRB_LDLIBS := -l:lib_lightgbm.so -l:libmongocxx.so -l:libwebcachesim.so
LDLIBS += $(LRB_LDLIBS)

# -L$(LRB_DIRPATH)/install/mongocdriver/lib
LRB_LDDIR = -L$(LRB_DIRPATH)/install/lightgbm/lib -L$(LRB_DIRPATH)/install/mongocxxdriver/lib -L$(LRB_DIRPATH)/install/webcachesim/lib