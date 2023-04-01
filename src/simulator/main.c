#include "boost/program_options.hpp"

int main(int argc, char **argv) {
    // TODO: load config information first

    boost::program_options::options_description argument_desc("Allowed arguments:");

    // TODO: use config.variables
    int client_cnt = 0;

    argument_desc.add_options()
        ("help,h", "dump help information")
        ("version,v", "dump version of COVERED")
        ("clientcnt", boost::program_options::value<int>(&client_cnt)->default_value(10), "the number of clients")
        ("include-path,I", po::value< vector<string> >(), 
    "include path")
        ("input-file", po::value< vector<string> >(), "input file")
    ;

    boost::program_options::variables_map argument_info;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, argument_desc), argument_info);
    boost::program_options::notify(argument_info); 

    if (argument_info.count("help")) {
        std::cout << argument_desc << std::endl;
        return 1;
    }
    if (argument_info.count("version")) {
        // TODO: use config.version to replace the magic number 1.0
        std::cout << "Current version of COVERED: 1.0" << std::endl;
        return 1;
    }
    
    return 1;
}