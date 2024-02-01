#pragma once

#include "platform.h"

#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>


class BerkeleyNetwork : public puf::Network {
private:
    int sockfd;
    char ifName[IFNAMSIZ];

    struct sockaddr_ll remote_address;
    struct sockaddr_ll local_address;
    socklen_t addr_size;
    bool initialised;

    void set_promisc(bool enable = true);
    void get_local_endpoint();
    void set_timeout();

public:
    BerkeleyNetwork(const char* iface_name);
    void init() override;
    void send(uint8_t *buf, size_t bufSize) override;
    int receive(uint8_t *buf, size_t bufSize) override;
};