# segcache module

SEGCACHE_CPPFLAGS = -DOS_LINUX
CPPFLAGS += $(SEGCACHE_CPPFLAGS)

# hacked part (include/link path and libs)

# NOTE: NO need to update SEGCACHE_DIRPATH in scripts/repalce_dir.py, as we always use hacked version
SEGCACHE_DIRPATH = $(COVERED_DIRPATH)/cache/segcache
SEGCACHE_INCDIR = -I$(SEGCACHE_DIRPATH)/src -I$(SEGCACHE_DIRPATH)/deps/ccommon/include -I$(SEGCACHE_DIRPATH)/build
INCDIR += $(SEGCACHE_INCDIR)

SEGCACHE_LDDIR = -L$(SEGCACHE_DIRPATH)/build/ccommon/lib -L$(SEGCACHE_DIRPATH)/build/datapool -L$(SEGCACHE_DIRPATH)/build/hotkey -L$(SEGCACHE_DIRPATH)/build/storage/seg -L$(SEGCACHE_DIRPATH)/build/time -L$(SEGCACHE_DIRPATH)/build/util
LDDIR += $(SEGCACHE_LDDIR)

SEGCACHE_LDLIBS = -l:libseg.a -l:libccommon.so -l:libdatapool.a -l:libhotkey.a -l:libtime.a -l:libutil.a -l:libtime.a
LDLIBS += $(SEGCACHE_LDLIBS)