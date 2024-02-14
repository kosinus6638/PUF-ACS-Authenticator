#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <chrono>


#include "Berkeley_Network.h"
#include "Authentication_Server.h"
#include "Serial_Master.h"
#include "Options.h"

#include "errors.h"
#include "packets.h"


static volatile int keepGoing = 1;
void intHandler(int signum) {
    keepGoing = 0;
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
