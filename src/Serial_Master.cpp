#include "Serial_Master.h"
#include <iostream>
#include <iomanip>

#include <fcntl.h>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <thread>


constexpr char DELIM = ';';
constexpr char EOL  = '\x04';


struct SerialOptions {
    std::string port;
    int baudrate;
    int speedtest_seconds;
    int delay_us;
    int frame_size;
};


static SerialOptions global_options = {
    "ttyUSB0",
    115200,
    5,
    100,
    1522
};


static void print_serial_options(struct SerialOptions *opts=nullptr, bool is_config_menu=false) 
{
    struct SerialOptions *to_print = opts ? opts : &global_options;

    std::cout << (is_config_menu ? "" : "\nCurrent status:")
              << "\n" << (is_config_menu ? "(a) " : "") << std::setw(14) << std::left << "Port:" << to_print->port
              << "\n" << (is_config_menu ? "(b) " : "") << std::setw(14) << std::left << "Baudrate:" << to_print->baudrate
              << "\n" << (is_config_menu ? "(c) " : "") << std::setw(14) << std::left << "Seconds:" << to_print->speedtest_seconds
              << "\n" << (is_config_menu ? "(d) " : "") << std::setw(14) << std::left << "delay us:" << to_print->delay_us
              << "\n" << (is_config_menu ? "(e) " : "") << std::setw(14) << std::left << "frame size:" << to_print->frame_size
              << std::endl;
}


UserDialog::UserDialog() {
    main_menu.push_back( std::make_tuple("Show Status", "", STATUS) );
    main_menu.push_back( std::make_tuple("Set Options", "", CONFIG) );
    main_menu.push_back( std::make_tuple("Register",    "", REGISTER) );
    main_menu.push_back( std::make_tuple("Connect",     "", CONNECT) );
    main_menu.push_back( std::make_tuple("Speedtest",   "", SPEEDTEST) );
    main_menu.push_back( std::make_tuple("ESP Status",  "", ESP_STATUS) );
    main_menu.push_back( std::make_tuple("Reconnect",   "", RECONNECT) );
    main_menu.push_back( std::make_tuple("Exit",        "", EXIT) );
}


void UserDialog::draw_screen() {
    std::cout << "\nPlease choose your option:" << std::endl;

    for(auto i=0; i<main_menu.size(); ++i) {
        const auto& [description, default_val, _] = main_menu.at(i);

        // Draw line
        std::cout << "(" << i << ")\t" << description;
        if( !default_val.empty() ) {
            std::cout << " (=" << default_val << ")";
        }
        std::cout << std::endl;
    }
}


int UserDialog::get_user_input() {
    UserInput retval = UNKNOWN;
    std::string user_input;

    draw_screen();
    std::cout << "Please enter (enter=0): ";

    try {
        std::getline(std::cin, user_input);
        if(user_input == "q" || user_input == "exit" || user_input == "quit") {
            retval = EXIT;
        } else {
            int user_input_no = user_input.empty() ? 0 : std::stoi(user_input);
            retval = std::get<2>(main_menu.at( user_input_no ));
        }

    } catch(const std::exception &e) {
        std::cerr << e.what() << "\n";
    }

    return retval;
}


int UserDialog::config_options() {
    std::string user_input;
    char option_to_change;
    bool done = false;

    struct SerialOptions new_opts = global_options;

    while(!done) {

        print_serial_options(&new_opts, true);
        std::cout << "Which option would you like to change?: ";

        std::getline(std::cin, user_input);
        if( user_input.empty() ) continue;
        option_to_change = user_input[0]; 

        switch(option_to_change) {
            case 'a':
                std::cout << "New port name: ";
                std::getline(std::cin, user_input);
                new_opts.port = user_input;
                break;

            case 'b':
                std::cout << "New baudrate: ";
                std::getline(std::cin, user_input);
                try {
                    new_opts.baudrate = std::stoi(user_input);
                } catch(const std::exception &e) {
                    std::cerr << "Error: " << e.what() << " Aborting!\n";
                }
                break;

            case 'c':
                std::cout << "New Speedtest time [s]: ";
                std::getline(std::cin, user_input);
                try {
                    new_opts.speedtest_seconds = std::stoi(user_input);
                } catch(const std::exception &e) {
                    std::cerr << "Error: " << e.what() << " Aborting!\n";
                }
                break;

            case 'd':
                std::cout << "New delay [us]: ";
                std::getline(std::cin, user_input);
                try {
                    new_opts.delay_us = std::stoi(user_input);
                } catch(const std::exception &e) {
                    std::cerr << "Error: " << e.what() << " Aborting!\n";
                }
                break;

            case 'e':
                std::cout << "New frame size [bytes]: ";
                std::getline(std::cin, user_input);
                try {
                    new_opts.frame_size = std::stoi(user_input);
                } catch(const std::exception &e) {
                    std::cerr << "Error: " << e.what() << " Aborting!\n";
                }
                break;

            default:
                done = true;
        }
    }

    global_options = new_opts;
    return 0;
}


/*-------------------------------------------------------------------------------------------------*/


SerialMaster::SerialMaster(std::string port_) : connected(false) {
    global_options.port = port_;
    setup();
}


void SerialMaster::reconnect() {
    setup();
}


void SerialMaster::setup() {
    std::string devdir = "/dev/";
    devdir += global_options.port;

    std::cout << "Connecting to port " << devdir << std::endl;

    /* Open port */
    serial_port = open(devdir.c_str(), O_RDWR);
    if(serial_port < 0) {
        printf("Error %d from open: %s\n", errno, std::strerror(errno));
        return;
    }

    /* Configure port */
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if( tcgetattr(serial_port, &tty) != 0 ) {
        printf("Error %d from tcgetattr: %s\n", errno, std::strerror(errno));
        return;
    }

    cfsetospeed(&tty, (speed_t)B115200);
    cfsetispeed(&tty, (speed_t)B115200);

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~ICANON;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 50;
    tty.c_cflag |= CREAD | CLOCAL;

    cfmakeraw(&tty);
    tcflush(serial_port, TCIFLUSH);

    if(tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        printf("Error %d from tcsetattr: %s\n", errno, std::strerror(errno));
        return;
    }

    connected = true;
}


int SerialMaster::serial_write(const char *cmd) {
    size_t n = 0, n_written = 0;
    size_t cmdLen = strlen(cmd);

    do {
        n = write(serial_port, (cmd+n_written), 1);
        n_written += n;
    } while(n>0 && n_written<cmdLen);

    write(serial_port, &EOL, 1);

    return n_written;
}


int SerialMaster::serial_read(char *cmd, size_t cmdLen) {
    char buf = '\0';
    size_t n = 0, n_written = 0;

    do {
        if( (n = read(serial_port, &buf, 1)) == 1 ) {
            cmd[n_written++] = buf;
            n_written %= cmdLen;    // Dirty little hack to prevent buffer overflows
        }
    } while(n>0 && buf != EOL);

    --n_written;

    cmd[n_written] = '\0';
    return n_written;  // Returns no nullbyte but includes EOL
}


void SerialMaster::show_status() {
    print_serial_options();
    std::cout << std::setw(14) << std::left << "Connected:" << std::boolalpha << connected << std::endl;
}


void SerialMaster::slave_connect() {

    const char cmd[] = "connect"; 
    char rcv_buf[512];
    memset(rcv_buf, 0, sizeof(rcv_buf));

    serial_write(cmd);
    serial_read(rcv_buf, sizeof(rcv_buf));

    if( strncmp(rcv_buf, "connect_ok\r", sizeof(rcv_buf)) != 0 ) {
        printf("Error connecting: %s\n", rcv_buf);
    }

    puts("Received connect_ok");
}


void SerialMaster::slave_status() {
    const char cmd[] = "status"; 
    char rcv_buf[512];
    memset(rcv_buf, 0, sizeof(rcv_buf));

    serial_write(cmd);
    serial_read(rcv_buf, sizeof(rcv_buf));

    puts(rcv_buf);
}


void SerialMaster::slave_sign_up() {
    const char cmd[] = "register"; 
    char rcv_buf[512];
    serial_write(cmd);
    serial_read(rcv_buf, sizeof(rcv_buf));
    puts(rcv_buf);

    if( strncmp(rcv_buf, "register_ok", sizeof(rcv_buf)) != 0 ) {
        puts("Error register");
    }
    puts("Received register_ok");
}


void SerialMaster::slave_speedtest() {
    char cmd[512] = "speedtest";
    char rcv_buf[512];

    memset(rcv_buf, 0, sizeof(rcv_buf));

    serial_write(cmd);
    serial_read(rcv_buf, sizeof(rcv_buf));

    if( strncmp(rcv_buf, "speedtest_ok", sizeof(rcv_buf)) != 0 ) {
        printf("Wrong answer: %s\n", rcv_buf);
        return;
    }

    memset(cmd, 0, sizeof(cmd));
    memset(rcv_buf, 0, sizeof(rcv_buf));

    /* Build command */
    snprintf(cmd, sizeof(cmd), "%c%d%c%d%c%d", 
        DELIM, global_options.speedtest_seconds, DELIM, 
        global_options.delay_us, DELIM, global_options.frame_size);

    serial_write(cmd);
    serial_read(rcv_buf, sizeof(rcv_buf));

    puts(rcv_buf);
}