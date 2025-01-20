#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <filesystem>
#include <vector>
#include <cstring>

using boost::asio::ip::tcp;
namespace fs = std::filesystem;

const int PORT = 1234;
const int VERSION = 1;

const std::string SERVER_DIRECTORY = "C:\\bakcupsrv";

struct Payload {
	uint32_t payload_size = 0;
	//std::vector<char> payload;
	std::vector<uint8_t> payload;
};


struct Client_Protocol {
    uint32_t user_id;
    uint8_t version;
    uint8_t op;
    uint16_t name_len;
    std::string file_name;
	Payload payload;

public:
    uint16_t get_name_len() const { return name_len; } //used for debugging
	
};

struct Server_Protocol {
	uint8_t version = VERSION;
	uint16_t status;
	uint16_t name_len = 0;
	std::string file_name;
	Payload payload;

public:
	std::vector<uint8_t> serialize();
	void set_payload(std::string files);
};

enum OP_code {
	FILE_BACKUP = 100, // Save file backup.All fields should be valid.
	FILE_RESTORE = 200, // Restore a file.size, payload unused.
	FILE_DELETE = 201, // Delete a file.size, payload unused.
	FILE_DIR = 202 // List all client's files. name_len, filename, size, payload unused.
};

enum status_code {
	FILE_RESTORE_OK = 210, // File restore successful.size, payload fields unused.
	FILE_DIR_OK = 211, // File list successful, all fields used.
	FILE_BACKUP_OK = 212, // File backup successful / File delete successful.size, payload fields unused
	ERR_NOT_EXIST = 1001, // File does not exist.size, payload fields unused.
	ERR_NO_FILES = 1002, // No files to list.size, payload fields unused.
	ERR_GENERAL = 1003 // General error.only version and staues fields are used
};

Client_Protocol parse_client_protocol(tcp::socket& socket);
void handle_client_protocol(tcp::socket& socket, const Client_Protocol& prot);

bool is_little_endian(); //checks if the system work in little endian

void get_ok_msg(tcp::socket& socket); // Wait for acknowledgment from the client before proceeding and closing the socket

//These functions convert little-endian values to the host's native byte order.
uint32_t le32toh(uint32_t val);
uint16_t le16toh(uint16_t val);

uint16_t htole16(uint16_t val);
uint32_t htole32(uint32_t val);