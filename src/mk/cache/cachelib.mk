# cachelib module

# NOTE: include path, lib path, and most libs of Cachelib have been added by src/mk/lib/cachebench.mk, here we only add the rest libs for Cachelib
# NOTE: use -lrt to support shm_open and shm_unlink for shared memory
CACHELIB_LDLIBS := -l:libthriftcpp2.so -l:libthriftmetadata.so -l:libthriftprotocol.so -lrt
LDLIBS += $(CACHELIB_LDLIBS)

# hacked part
# NOTE: CACHELIB uses template class -> NO need to compile into binary objects and static/dynamic libraries