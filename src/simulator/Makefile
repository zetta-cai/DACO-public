include ../../common/mk/recipes.mk

DEPS :=
CLEAN :=

##############################################################################
# RocksDB module

#ROCKSDB_INCDIR += -I../rocksdb-6.22.1/include
#INCDIR += $(ROCKSDB_INCDIR)

#DEPS += rocksdb_wrapper.d rocksdb_wrapper.shared.d
#CLEAN += rocksdb_wrapper.o rocksdb_wrapper.shared.o
#ROCKSDB_OBJECTS += rocksdb_wrapper.o
#ROCKSDB_SHARED_OBJECTS += rocksdb_wrapper.shared.o

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
-include $(DEPS)
