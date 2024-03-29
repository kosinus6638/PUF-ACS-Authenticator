#include "Options.h"
#include <boost/program_options.hpp>
#include <iostream>

Options get_options(int argc, char** argv) {
    namespace po = boost::program_options;
    Options retval;
    po::options_description opts_desc("Allowed options");
    po::positional_options_description p;
    po::variables_map vm;

    auto print_help = [&opts_desc]() {
        std::cout << "Usage: au [interface] [file]" << std::endl;
        std::cout << opts_desc << std::endl;
    };

    opts_desc.add_options()
        ("file,f", po::value<std::string>(&retval.resource_file)->default_value("Supplicant.csv"), "Resource file")
        ("help,h", "Print this help")
        ("interface,I", po::value<std::string>(&retval.iface_name)->default_value("enp4s0"), "Bind to interface")
        ("payload_bufsize,p", po::value<int>(&retval.payload_bufsize)->default_value(3), "Payload buffer size")
        ("rounds,r", po::value<int>(&retval.rounds)->default_value(10), "Number of rounds")
        ("save_on_edit,s", "Save resource file on edit")
        ("verbose,v", "Verbose output")
    ;

    p.add("interface", 1);
    p.add("file", 2);

    try {
        po::store(po::command_line_parser(argc, argv).options(opts_desc).positional(p).run(), vm);
        po::notify(vm);
    } catch(const po::error &e) {
        print_help();
        throw std::runtime_error(e.what());
    }

    if(vm.count("help")) {
        print_help();
        exit(EXIT_SUCCESS);
    }

    retval.verbose = vm.count("verbose");
    retval.save_on_edit = vm.count("save_on_edit");
    return retval;
}