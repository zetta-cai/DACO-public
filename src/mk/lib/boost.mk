# boost module

BOOST_DIRPATH := lib/boost_1_81_0

BOOST_LDDIR := -L$(BOOST_DIRPATH)/install/lib
LDDIR += $(BOOST_LDDIR)

# Boost libs from $(BOOST_DIRPATH)/install/lib
BOOST_LDLIBS := -l:libboost_program_options.a -l:libboost_json.a -l:libboost_system.a -l:libboost_filesystem.a -l:libboost_stacktrace_backtrace.a -l:libboost_thread.a
# Third-party libs from /usr/lib/gcc/x86_64-linux-gnu/9
BOOST_LDLIBS += -lbacktrace
LDLIBS += $(BOOST_LDLIBS)

BOOST_INCDIR := -I$(BOOST_DIRPATH)/install/include
INCDIR += $(BOOST_INCDIR)