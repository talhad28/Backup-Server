#pragma once
#include "Server.hpp"
#include "Handle_Files.hpp"



//checks if the system work in little endian
bool is_little_endian() {
    uint16_t test = 1;
    return *reinterpret_cast<uint8_t*>(&test) == 1;
}


//These functions convert little-endian values to the host's native byte order.
uint32_t le32toh(uint32_t val) {
    return (val & 0xFF) << 24 | (val & 0xFF00) << 8 | (val & 0xFF0000) >> 8 | (val & 0xFF000000) >> 24;
}

uint16_t le16toh(uint16_t val) {
    return (val & 0xFF) << 8 | (val & 0xFF00) >> 8;
}


Protocol parse_protocol(tcp::socket &socket) {
    Protocol prot;
    boost::asio::streambuf buffer;

    // Read the fixed-size header (12 bytes: 4 + 1 + 1 + 2 + 4)
    boost::asio::read(socket, buffer, boost::asio::transfer_exactly(12));
    std::istream is(&buffer);
    
    char header[12];
    is.read(header, 12);

    // Parse the header fields
    memcpy(&prot.user_id, header, 4);
    if (!is_little_endian())
        prot.user_id = le32toh(prot.user_id); // Convert from little-endian to host byte order

    prot.version = header[4];
    prot.op = header[5];

    memcpy(&prot.name_len, header + 6, 2);
    if(!is_little_endian())
        prot.name_len = le16toh(prot.name_len); // Convert from little-endian to host byte order

    //read the payload size
    memcpy(&prot.payload_size, header + 8, 4);
    if (!is_little_endian())
        prot.payload_size = le32toh(prot.payload_size);

    // Read the file name
    boost::asio::read(socket, buffer, boost::asio::transfer_exactly(prot.name_len));
    std::vector<char> file_name(prot.name_len);
    is.read(file_name.data(), prot.name_len);
    prot.file_name = std::string(file_name.data(), prot.name_len);

    std::cout <<"File name:" << prot.file_name << std::endl;


    // Read the remaining payload
    /*boost::asio::read(socket, buffer, boost::asio::transfer_exactly(prot.payload_size));
    prot.payload.resize(prot.payload_size);
    is.read(prot.payload.data(), prot.payload_size);*/
    

    return prot;
}


void handle_protocol(tcp::socket &socket, const Protocol& prot) {
    std::string user_dir = SERVER_DIRECTORY + "\\" + std::to_string(prot.user_id);
    // Create a directory for the user ID
    if (!fs::exists(user_dir)) {
        fs::create_directory(user_dir);
    }

    if (prot.op == 100) {
        save_file(socket, prot.file_name, user_dir, prot.payload_size);
    }
    else if (prot.op == 200) {
        send_file(socket, prot.file_name, user_dir);
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

        //boost::asio::streambuf buffer;
        //boost::asio::read_until(socket, buffer, '\0'); // Read until EOF or close

        Protocol prot = parse_protocol(socket);
        handle_protocol(socket, prot);
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
