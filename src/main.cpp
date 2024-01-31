#include <stdlib.h>
#include <signal.h>
#include <iostream>

#include "Berkeley_Network.h"
#include "Authentication_Server.h"
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

    puf::print_version();

    try {
        BerkeleyNetwork net; 
        AuthenticationServerImpl as; 

        Authenticator au(net, as);
        au.init();

        while(keepGoing) {
            try {
                au.accept();
            } catch(const NetworkException& e) {
            } catch(const Exception& e) {
                puts(e.what());
            }
        }
    } catch(const Exception& e) {
        puts(e.what());
    }


    puts("Feddisch");
    return 0;
}