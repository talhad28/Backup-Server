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

//checks if the system work in little endian
bool is_little_endian() {
    uint16_t test = 1;
    return *reinterpret_cast<uint8_t*>(&test) == 1;
}

struct Protocol {
    uint32_t user_id;
    uint8_t version;
    uint8_t op;
    uint16_t name_len;
    std::string file_name;
    std::vector<char> payload;

public:
    uint16_t get_name_len() const { return name_len; } //used for debugging
};


//These functions convert little-endian values to the host's native byte order.
uint32_t le32toh(uint32_t val) {
    return (val & 0xFF) << 24 | (val & 0xFF00) << 8 | (val & 0xFF0000) >> 8 | (val & 0xFF000000) >> 24;
}

uint16_t le16toh(uint16_t val) {
    return (val & 0xFF) << 8 | (val & 0xFF00) >> 8;
}


Protocol parse_protocol(boost::asio::streambuf& buffer) {
    Protocol prot;
    std::istream is(&buffer);

    // Read the fixed-size header (8 bytes: 4 + 1 + 1 + 2)
    char header[8];
    is.read(header, 8);

    // Parse the header fields
    memcpy(&prot.user_id, header, 4);
    if (!is_little_endian)
        prot.user_id = le32toh(prot.user_id); // Convert from little-endian to host byte order

    prot.version = header[4];
    prot.op = header[5];

    memcpy(&prot.name_len, header + 6, 2);
    if(!is_little_endian)
        prot.name_len = le16toh(prot.name_len); // Convert from little-endian to host byte order

    // Read the file name
    char file_name[1024] = { 0 };
    is.read(file_name, prot.name_len);
    if (is.gcount() != prot.name_len) {
        throw std::runtime_error("Failed to read file name from buffer.");
    }
    prot.file_name = std::string(file_name, prot.name_len);

    // Read the remaining payload
    prot.payload.assign(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>());

    return prot;
}


void handle_protocol(const Protocol& prot) {
    if (prot.op == 200) {
        // Create a directory for the user ID
        std::string user_dir = SERVER_DIRECTORY + "\\" + std::to_string(prot.user_id);
        if (!fs::exists(user_dir)) {
            fs::create_directory(user_dir);
        }

        // Save the file in the user directory
        std::string full_file_path = user_dir + "\\" + prot.file_name;
        std::ofstream output_file(full_file_path, std::ios::binary);
        if (output_file.is_open()) {
            output_file.write(prot.payload.data(), prot.payload.size());
            output_file.close();
            std::cout << "File saved to: " << full_file_path << std::endl;
        }
        else {
            std::cerr << "Failed to create file: " << full_file_path << std::endl;
        }
    }
    else {
        std::cerr << "Unknown operation: " << prot.op << std::endl;
    }
}

int main() {
    try {
        // Ensure the server directory exists
        if (!fs::exists(SERVER_DIRECTORY)) {
            fs::create_directory(SERVER_DIRECTORY);
        }

        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), PORT));

        std::cout << "Server is listening on port " << PORT << std::endl;

        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "Client connected!" << std::endl;

        boost::asio::streambuf buffer;
        boost::asio::read_until(socket, buffer, '\0'); // Read until EOF or close

        Protocol prot = parse_protocol(buffer);
        handle_protocol(prot);
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
