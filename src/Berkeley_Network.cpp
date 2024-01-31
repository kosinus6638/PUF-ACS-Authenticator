#include "Berkeley_Network.h"
#include "errors.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>


BerkeleyNetwork::BerkeleyNetwork() : 
    initialised(false), 
    remote_address{0}, 
    local_address{0}, 
    addr_size(sizeof(struct sockaddr_ll))
{}


void BerkeleyNetwork::get_local_endpoint() {
    memset( static_cast<void*>(&local_address), 0, sizeof(struct sockaddr_ll) );
	struct ifreq if_idx;    

    /* Get interface index */
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
        throw puf::NetworkException( strerror(errno) );
    }

    /* Here it is important to set sll_ifindex, sll_family and sll_protocol */
    local_address.sll_family = AF_PACKET;
    local_address.sll_ifindex = if_idx.ifr_ifindex;
    local_address.sll_protocol = htons(ETH_P_ALL);
}


void BerkeleyNetwork::set_promisc(bool enable) {
    struct packet_mreq mreq = {0};
    int action = enable ? PACKET_ADD_MEMBERSHIP : PACKET_DROP_MEMBERSHIP;

    mreq.mr_ifindex = local_address.sll_ifindex;
    mreq.mr_type = PACKET_MR_PROMISC;
    if(setsockopt(sockfd, SOL_PACKET, action, &mreq, sizeof(mreq)) != 0) {
        throw puf::NetworkException( strerror(errno) );
    }
}


void BerkeleyNetwork::set_timeout() {
    struct timeval timeout;
    timeout.tv_sec = NETWORK_TIMEOUT_MS/1000;
    timeout.tv_usec = 0;
    if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0 ) {
        throw puf::NetworkException( strerror(errno) );
    }
}


void BerkeleyNetwork::init() {
    if(initialised) return;

    strcpy(ifName, DEFAULT_IF);

    // Create raw packet socket
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, 0)) == -1) {
        throw puf::NetworkException( strerror(errno) );
    }    

    set_timeout();
    get_local_endpoint();

    // Bind to interface
    if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&local_address), sizeof(struct sockaddr_ll)) == -1) {
        throw puf::NetworkException( strerror(errno) );
    }

    set_promisc();

    printf("Bound to interface %s with index %d\n", ifName, local_address.sll_ifindex);
    initialised = true;
}


void BerkeleyNetwork::send(uint8_t *buf, size_t bufSize) {
    if( sendto(sockfd, buf, bufSize, 0, reinterpret_cast<struct sockaddr*>(&local_address), sizeof(struct sockaddr_ll))  < 0) {
        throw puf::NetworkException( strerror(errno) );
    }
}


int BerkeleyNetwork::receive(uint8_t *buf, size_t bufSize) {
    int n;
    if( (n = recvfrom(sockfd, buf, bufSize-1, 0, reinterpret_cast<struct sockaddr*>(&remote_address), &addr_size)) < 0) {
        throw puf::NetworkException("Timeout");
    }
    buf[n] = 0;
    return n;
}