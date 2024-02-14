#pragma once

#include <string>

typedef struct Options {
    std::string iface_name;
    std::string resource_file;
    int payload_bufsize;
    int rounds;
    bool verbose;
    bool save_on_edit;
} Options;


Options get_options(int argc, char** argv);