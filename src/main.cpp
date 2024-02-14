#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <chrono>

#include <boost/program_options.hpp>

#include "Berkeley_Network.h"
#include "Authentication_Server.h"
#include "Serial_Master.h"

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
    bool save_on_edit;
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


    try {

    BerkeleyNetwork net( opts.iface_name.c_str() ); 
    AuthenticationServerImpl as( opts.resource_file.c_str(), opts.save_on_edit ); 
    Authenticator au(net, as);

    SerialMaster serial_master("ttyUSB0");
    UserDialog user_dialog;

    au.init();

    uint8_t buffer[1522];
    size_t n;
    size_t received_bytes = 0;


    auto speedtest = [&]() {
        using namespace std::chrono;
        bool speedtesting = true;

        high_resolution_clock::time_point start, end;
        PUF_Performance pp;

        while(speedtesting) {
            n = net.receive(buffer, sizeof(buffer));
            if(deduce_type(buffer, sizeof(buffer)) != PUF_PERFORMANCE_E) {
                continue;
            }
            
            pp.from_binary(buffer, n);
            switch( pp.get_data()[0] ) {
                case 'F':
                    start = high_resolution_clock::now();
                case 'H':
                    if( au.validate(pp, true) ) {
                        received_bytes += pp.header_len();
                    }
                    break;

                case 'L':
                    end = high_resolution_clock::now();
                    received_bytes += pp.header_len();
                    speedtesting = false;
                    break;

                default:
                    std::cerr << "Default statement reached\n";
                    ;
            }
        }
        auto duration = end-start;
        double to_mbit_s = 8.0 / 1024;
        std::cout << "Received for\t" << duration.count() << " ms" << std::endl;
        std::cout << "Received\t" << received_bytes << " bytes" << std::endl;
        std::cout << "Average speed:\t" << ((double)received_bytes/(double)duration.count()) * to_mbit_s << " mbit/s" << std::endl;
    };


    while(keepGoing) {
        switch(user_dialog.get_user_input()) {
            case STATUS:
                serial_master.show_status();
                break;

            case CONFIG:
                user_dialog.config_options();
                break;

            case CONNECT:
                serial_master.slave_connect();
                n = net.receive(buffer, sizeof(buffer));
                if( deduce_type(buffer, sizeof(buffer)) != PUF_CON_E ) {
                    break;
                }

                if(au.accept(buffer, n) != 0) {
                    std::cout << "Rejected" << std::endl;
                } else {
                    std::cout << "Accepted" << std::endl;
                }
                break;

            case UserInput::REGISTER:
                serial_master.slave_sign_up();
                au.sign_up();
                break;

            case SPEEDTEST:
                serial_master.slave_speedtest();
                speedtest();                
                break;

            case RECONNECT:
                serial_master.reconnect();
                break;

            case ESP_STATUS:
                serial_master.slave_status();
                break;

            case EXIT:
                keepGoing = false;
                break;

            case UNKNOWN:
                std::cout << "Unkown input" << std::endl;
                break;

            default:
                ;
        }
    }

    } catch(const Exception &e) {
        puts(e.what());
    }

    puts("Feddisch");
    return 0;
}
