#pragma once

#include <string>
#include <map>
#include "authenticator.h"

class SupplicantEntry {
private:
    int ctr;
    puf::MAC base_mac;
    puf::MAC hashed_mac;
    puf::ECP_Point A;
public:
    SupplicantEntry(std::string csv_row);
    SupplicantEntry(int ctr_, const puf::MAC& base_mac_, const puf::MAC& hashed_mac_, const puf::ECP_Point& A_);
    std::string to_string() const;
    void decrease_counter();
    void hash_mac();
    friend class AuthenticationServerImpl;
};


class AuthenticationServerImpl : public puf::AuthenticationServer {
    std::map<uint64_t, SupplicantEntry> entries;
public:
    void fetch(const char* url=DEFAULT_RESOURCE);
    void sync(const char* url=DEFAULT_RESOURCE);
    void store(const puf::MAC& base_mac, const puf::ECP_Point& A, const puf::MAC& hashed_mac, int ctr=DEFAULT_COUNTER);
    puf::QueryResult query(const puf::MAC& hashed_mac);
};