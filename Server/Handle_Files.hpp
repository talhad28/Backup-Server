#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <filesystem>
#include <vector>
#include <cstring>

const size_t PACKET_SIZE = 1024 * 1024; // 1 MB per chunk
const std::string OK_MSG = "OK";

using boost::asio::ip::tcp;
namespace fs = std::filesystem;

class File_transfer {
public:
	static void send_file(tcp::socket& socket, const std::string& user_dir, const std::string& file_name);
	static void save_file(tcp::socket& socket, const std::string& file_name, size_t payload_size);
	static void return_file_list(tcp::socket& socket, const std::string& user_dir);
	static void delete_file(tcp::socket& socket, const std::string& file_name);
};
