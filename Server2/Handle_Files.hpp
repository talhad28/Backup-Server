#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <filesystem>
#include <vector>
#include <cstring> // For memcpy

using boost::asio::ip::tcp;
namespace fs = std::filesystem;

void receive_file_in_chunks(tcp::socket& socket, const std::string& file_name, size_t payload_size);

void save_file(tcp::socket& socket, const std::string& file_name, std::string& user_dir, size_t payload_size);

void send_file(tcp::socket& socket, const std::string& file_name, std::string& user_dir);