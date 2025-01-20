#include "Protocol.hpp"
#include "Handle_Files.hpp"


Client_Protocol parse_client_protocol(tcp::socket& socket) {
    Client_Protocol prot;
    boost::asio::streambuf buffer;

    // Read the fixed-size header (8 bytes: 4 + 1 + 1 + 2)
    boost::asio::read(socket, buffer, boost::asio::transfer_exactly(8));
    std::istream is(&buffer);

    char header[8];
	char payload_size[4];
    is.read(header, 8);

    // Parse the header fields
    memcpy(&prot.user_id, header, 4);
    if (!is_little_endian())
        prot.user_id = le32toh(prot.user_id); // Convert from little-endian to host byte order

    prot.version = header[4];
    prot.op = header[5];

    memcpy(&prot.name_len, header + 6, 2);
    if (!is_little_endian())
        prot.name_len = le16toh(prot.name_len); // Convert from little-endian to host byte order

    // Read the file name
    boost::asio::read(socket, buffer, boost::asio::transfer_exactly(prot.name_len));
    std::vector<char> file_name(prot.name_len);
    is.read(file_name.data(), prot.name_len);
    prot.file_name = std::string(file_name.data(), prot.name_len);

    //read the payload size
    boost::asio::read(socket, buffer, boost::asio::transfer_exactly(4));
	is.read(payload_size, 4);
	memcpy(&prot.payload.payload_size, payload_size, 4);
    if (!is_little_endian())
        prot.payload.payload_size = le32toh(prot.payload.payload_size);

    

    return prot;
}

std::vector<uint8_t> Server_Protocol::serialize() {
    std::vector<uint8_t> buffer;

    // Version and status (1 byte each)
    buffer.push_back(version);
    uint16_t status_le = htole16(status);
    size_t current_pos = buffer.size();
    buffer.resize(buffer.size() + sizeof(uint16_t));
    memcpy(&buffer[buffer.size() - sizeof(uint16_t)], &status_le, sizeof(uint16_t));

    // Name length (2 bytes) - convert to little endian
    uint16_t name_len_le = htole16(name_len);
    buffer.resize(buffer.size() + sizeof(uint16_t));
    current_pos = buffer.size();
    memcpy(&buffer[buffer.size() - sizeof(uint16_t)], &name_len_le, sizeof(uint16_t));

    // File name
    buffer.insert(buffer.end(), file_name.begin(), file_name.end());

    // Payload size (4 bytes) - convert to little endian
    uint32_t size_le = is_little_endian() ? payload.payload_size : htole32(payload.payload_size);
    current_pos = buffer.size();
    buffer.resize(buffer.size() + sizeof(uint32_t));
    memcpy(&buffer[buffer.size() - sizeof(uint32_t)], &size_le, sizeof(uint32_t));

    // Payload data
    buffer.insert(buffer.end(), payload.payload.begin(), payload.payload.end());

    return buffer;
}

void Server_Protocol::set_payload(std::string files) {
    std::vector<uint8_t> vec(files.begin(), files.end());
    payload.payload = vec;
    payload.payload_size = static_cast<uint32_t>(files.length());
}

void handle_client_protocol(tcp::socket& socket, const Client_Protocol& prot) {
    std::string user_dir = SERVER_DIRECTORY + "\\" + std::to_string(prot.user_id);
    // Create a directory for the user ID if does not exist
    if (!fs::exists(user_dir)) {
        fs::create_directory(user_dir);
    }
    std::string full_file_path = user_dir + "\\" + prot.file_name;

    if (prot.op == OP_code(FILE_BACKUP)) {
        std::cout << "File name:" << prot.file_name << std::endl;
        File_transfer::save_file(socket, full_file_path, prot.payload.payload_size);
    }
    else if (prot.op == OP_code(FILE_RESTORE)) {
        File_transfer::send_file(socket, user_dir, prot.file_name);
    }
    else if (prot.op == OP_code(FILE_DIR)) {
        File_transfer::return_file_list(socket, user_dir);
    }
    else if (prot.op == OP_code(FILE_DELETE)) {
        File_transfer::delete_file(socket, full_file_path);
    }
    else {
        std::cerr << "Unknown operation: " << prot.op << std::endl;
    }
    //socket.close();
}


void get_ok_msg(tcp::socket& socket) {
    try {
        boost::asio::streambuf buffer;
        boost::asio::read(socket, buffer, boost::asio::transfer_exactly(2));
        std::istream is(&buffer);
        std::string response;
        std::getline(is, response);

        if (response != OK_MSG) {
            std::cerr << "Error: acknowledge reponse unknown: " << response << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "Error while waiting for acknowledgment: " << e.what() << std::endl;
    }
}

bool is_little_endian() {
    uint16_t test = 1;
    return *reinterpret_cast<uint8_t*>(&test) == 1;
}

uint32_t le32toh(uint32_t val) {
    return (val & 0xFF) << 24 | (val & 0xFF00) << 8 | (val & 0xFF0000) >> 8 | (val & 0xFF000000) >> 24;
}

uint16_t le16toh(uint16_t val) {
    return (val & 0xFF) << 8 | (val & 0xFF00) >> 8;
}

uint16_t htole16(uint16_t val) {
    if (is_little_endian()) {
        return val;
    }
    return (val << 8) | (val >> 8);
}

uint32_t htole32(uint32_t val) {
    if (is_little_endian()) {
        return val;
    }
    return ((val << 24) & 0xFF000000) |
        ((val << 8) & 0x00FF0000) |
        ((val >> 8) & 0x0000FF00) |
        ((val >> 24) & 0x000000FF);
}

