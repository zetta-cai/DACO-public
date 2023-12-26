# rocksdb module

ROCKSDB_DIRPATH := $(LIBRARY_DIRPATH)/rocksdb-8.1.1

ROCKSDB_LDDIR := -L$(ROCKSDB_DIRPATH)
LDDIR += $(ROCKSDB_LDDIR)

# RocksDB libs from $(ROCKSDB_DIRPATH)
ROCKSDB_LDLIBS := -lrocksdb
# Third-party libs from /usr/lib/x86_64-linux-gnu
#ROCKSDB_LDLIBS += -l:libz.a -l:libbz2.a -l:liblz4.a -l:libzstd.a -l:libsnappy.a -ldl
ROCKSDB_LDLIBS += -l:libz.so -l:libbz2.so -l:liblz4.so -l:libzstd.so -l:libsnappy.so -ldl
LDLIBS += $(ROCKSDB_LDLIBS)

ROCKSDB_INCDIR := -I$(ROCKSDB_DIRPATH)/include
INCDIR += $(ROCKSDB_INCDIR)