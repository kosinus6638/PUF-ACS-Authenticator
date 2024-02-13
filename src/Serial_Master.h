#pragma once

#include <string>
#include <vector>
#include <tuple>

enum UserInput {
    STATUS,
    CONFIG,
    CONNECT,
    REGISTER,
    SPEEDTEST,
    RECONNECT,
    ESP_STATUS,
    EXIT,
    UNKNOWN
};


class UserDialog {
private:
    using Entry = std::tuple<std::string, std::string, UserInput>;

    std::vector<Entry> main_menu;
    void draw_screen();

public:
    UserDialog();
    int get_user_input();
    int config_options();
};


class SerialMaster {
private:
    int serial_port;
    bool connected;

    int serial_write(const char *cmd, size_t cmdLen);
    int serial_read(char *cmd, size_t cmdLen);

    void setup();

public:
    SerialMaster(std::string port_);

    /* Instructions sent to slave */
    void slave_connect();
    void slave_sign_up();
    void slave_status();
    void slave_speedtest();

    void reconnect();
    void show_status();
};