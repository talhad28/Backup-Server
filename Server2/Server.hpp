#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <filesystem>
#include <vector>
#include <cstring> // For memcpy

using boost::asio::ip::tcp;
namespace fs = std::filesystem;

const int PORT = 1234;
const std::string SERVER_DIRECTORY = "C:\\server";

struct Protocol {
    uint32_t user_id;
    uint8_t version;
    uint8_t op;
    uint16_t name_len;
    std::string file_name;
    uint32_t payload_size;
    std::vector<char> payload;

public:
    uint16_t get_name_len() const { return name_len; } //used for debugging
};

Protocol parse_protocol(tcp::socket& socket);
void handle_protocol(const Protocol& prot);