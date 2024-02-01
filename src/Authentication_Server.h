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
    std::string url;
    std::map<uint64_t, SupplicantEntry> entries;
    bool save_on_edit_;
public:
    AuthenticationServerImpl(std::string url_, bool save_on_edit=false);
    void fetch() override;
    void sync() override;
    void store(const puf::MAC& base_mac, const puf::ECP_Point& A, puf::MAC& hashed_mac, int ctr) override;
    puf::QueryResult query(const puf::MAC& hashed_mac) override;
};