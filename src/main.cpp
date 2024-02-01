#include <stdlib.h>
#include <signal.h>
#include <iostream>

#include <boost/program_options.hpp>

#include "Berkeley_Network.h"
#include "Authentication_Server.h"
#include "errors.h"
#include "packets.h"


static volatile int keepGoing = 1;
void intHandler(int signum) {
    keepGoing = 0;
}


typedef struct Options {
    std::string iface_name;
    std::string resource_file;
    int payload_bufsize;
    int rounds;
    bool verbose;
} Options;


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
    return retval;
}


int main(int argc, char** argv) {
    using namespace puf;
    signal(SIGINT, intHandler);
    srand(1337);

    Options opts;
    try {
        opts = get_options(argc, argv);
    } catch(const std::runtime_error &e) {
        std::cerr << e.what() << "\n";
        exit(EXIT_FAILURE);
    }


        BerkeleyNetwork net( opts.iface_name.c_str() ); 
        AuthenticationServerImpl as( opts.resource_file.c_str() ); 
        Authenticator au(net, as);

        au.init();

        uint8_t buffer[1528];
        size_t n;

        while(keepGoing) {

        try {
            n = net.receive(buffer, sizeof(buffer));
            switch( deduce_type(buffer, n) ) {
                case PUF_CON_E:
                    if( au.accept(buffer, n) != 0) {
                        std::cout << "Rejected" << std::endl;
                    } else {
                        std::cout << "Accepted" << std::endl;
                    }
                    break;
                case PUF_PERFORMANCE_E:
                    break;
                case PUF_UNKNOWN_E:
                    break;
                default:
                    ;
            }
        } catch(const NetworkException& e) {
        } catch(const Exception &e) {
            puts(e.what());
        }
    }


    puts("Feddisch");
    return 0;
}