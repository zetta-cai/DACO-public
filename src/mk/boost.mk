# boost module

BOOST_LDDIR := -Llib/boost_1_81_0/install/lib
LDDIR += $(BOOST_LDDIR)
BOOST_LDLIBS := -l:libboost_program_options.a -l:libboost_json.a -l:libboost_system.a -l:libboost_filesystem.a
LDLIBS += $(BOOST_LDLIBS)

BOOST_INCDIR := -Ilib/boost_1_81_0/install/include
INCDIR += $(BOOST_INCDIR)