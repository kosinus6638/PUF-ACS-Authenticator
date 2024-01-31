#include "Authentication_Server.h"

#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>


constexpr char DELIM = ';';


/* --------------------------------------------- SupplicantEntry Implementation -----------------------------------*/

SupplicantEntry::SupplicantEntry(std::string csv_row) {
    std::istringstream iss(csv_row);
    std::vector<std::string> values;
    std::string to_insert;
    char delim_void = '\0';
    int idx = 0;

    // Split into separate strings
    while( std::getline(iss, to_insert, DELIM) ) {
        values.push_back(to_insert);
    }

    // Counter
    ctr = std::stoi(values.at(idx++));

    // Base MAC
    std::istringstream base_mac_stream(values.at(idx++));
    for(auto i=0; i<sizeof(base_mac.bytes); ++i) {
        int test;
        base_mac_stream >> std::hex >> test;
        base_mac_stream >> delim_void;
        base_mac.bytes[i] = static_cast<uint8_t>(test);
    }

    // A
    A.from_base64( reinterpret_cast<const uint8_t*>( values.at(idx++).c_str() ) );

    // Hashed MAC
    std::istringstream hashed_mac_stream(values.at(idx++));
    for(auto i=0; i<sizeof(hashed_mac.bytes); ++i) {
        int test;
        hashed_mac_stream >> std::hex >> test;
        hashed_mac_stream >> delim_void;
        hashed_mac.bytes[i] = static_cast<uint8_t>(test);
    }
}


SupplicantEntry::SupplicantEntry(int ctr_, const puf::MAC& base_mac_, const puf::MAC& hashed_mac_, const puf::ECP_Point& A_) :
        ctr(ctr_),
        base_mac(base_mac_),
        hashed_mac(hashed_mac_),
        A(A_)
    {}


std::string SupplicantEntry::to_string() const {
    std::ostringstream oss;

    // Counter
    oss << ctr << DELIM;

    // Base MAC
    for(auto i=0; i<sizeof(base_mac.bytes); ++i) {
        char delim = (i==sizeof(base_mac.bytes)-1) ? DELIM : ':'; 
        oss << std::hex << static_cast<int>(base_mac.bytes[i]);
        oss << delim;
    }

    // A
    oss << A.base64() << DELIM;

    // Hashed MAC
    for(auto i=0; i<sizeof(hashed_mac.bytes); ++i) {
        char delim = (i==sizeof(hashed_mac.bytes)-1) ? DELIM : ':'; 
        oss << std::hex << static_cast<int>(hashed_mac.bytes[i]);
        oss << delim;
    }

    oss << std::endl;
    return oss.str();
}


void SupplicantEntry::decrease_counter() {
    ctr--;
}


void SupplicantEntry::hash_mac() {
    hashed_mac.hash(1);
}



/* ------------------------------- AuthenticationServerImpl Implementation -----------------------------------*/

void AuthenticationServerImpl::fetch(const char* url) {
    std::ifstream ifs;
    std::string csv_row;
    bool failed = false;

    // Raise exception on error
    ifs.exceptions(std::ifstream::failbit);

    try {
        ifs.open(url);
        while( !ifs.eof() && std::getline(ifs, csv_row) )  {    // ToDo: Do not raise exception when EOF
            try {
                SupplicantEntry to_insert(csv_row);
                entries.insert( std::make_pair(to_insert.hashed_mac.to_u64(), to_insert) );
            } catch(...) {
                std::cerr << "Error creating entry: " << csv_row << '\n';
            }
        }
    } catch(const std::ios_base::failure &e) {
        std::cerr << e.what() << '\n';
        failed = true;
    }

    if(ifs.is_open()) ifs.close();
}



void AuthenticationServerImpl::sync(const char* url) {
    std::ofstream ofs;
    bool failed = false;

    ofs.exceptions(std::ofstream::failbit);
    try {
        ofs.open(url);
        for(const auto& [key, entry] : entries) {
            ofs << entry.to_string();
        }
    } catch(const std::ios_base::failure &e) {
        std::cerr << "Error saving entries: " << e.what() << '\n';
    }

    if(ofs.is_open()) ofs.close();
}


void AuthenticationServerImpl::store(const puf::MAC& base_mac, const puf::ECP_Point& A, 
    const puf::MAC& hashed_mac, int ctr) 
{
    entries.insert( std::make_pair(hashed_mac.to_u64(), SupplicantEntry(ctr, base_mac, hashed_mac, A)) );
}


puf::QueryResult AuthenticationServerImpl::query(const puf::MAC& hashed_mac) {
    if( auto it=entries.find(hashed_mac.to_u64()); it != entries.end() ) {
        it->second.decrease_counter();
        it->second.hash_mac();
        // return std::make_pair( it->second.A, it->second.base_mac );
    }
    return {};
}