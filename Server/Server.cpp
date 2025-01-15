#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <filesystem>

namespace fs = std::filesystem;



using boost::asio::ip::tcp;

const int PORT = 1234;
char FILE_NAME[1024] = "temp.pdf";
char temp[1024] = "temp.pdf";
const std::string SERVER_DIR = "C:\\SERVER";

void clear(char msg[]) {
for (int i = 0; i < 1024; i++)
    msg[i] = '\0';
}

void recive_files(tcp::socket &socket) {
// Buffer to hold received data
char buffer[1024] = { 0 };


//reads the file's name with the length given
char file_name_length_buffer[4];  // 4 bytes to store the file name length
boost::asio::read(socket, boost::asio::buffer(file_name_length_buffer, 4));
//extracting the length of the file name (sent by the client) from the raw buffer and converting it from network byte order to host byte order
uint32_t file_name_length = ntohl(*reinterpret_cast<uint32_t*>(file_name_length_buffer)); 

char file_name[1024] = { 0 };
boost::asio::read(socket, boost::asio::buffer(file_name, file_name_length));

std::string full_file_path = SERVER_DIR + "\\" + std::string(file_name);
std::ofstream output_file(full_file_path, std::ios::binary);
std::cout << "Receiving file: " << file_name << std::endl;


// Read data from the client
for (;;) {
    try {
        clear(buffer);
        size_t length = socket.read_some(boost::asio::buffer(buffer));

        // If we receive the EOF signal, stop receiving
        if (length > 0 && std::string(buffer, length) == "EOF") {
            std::cout << "File transfer complete!" << std::endl;
            break;
        }

        // Write the received chunk to the output file
        output_file.write(buffer, length);

        // Send acknowledgment to the client that it's ready for the next chunk
        std::string ready_message = "Ready for next chunk";
        socket.send(boost::asio::buffer(ready_message));
    }
    catch (const boost::system::system_error& e) {
        if (e.code() == boost::asio::error::eof) {
            // Client disconnected gracefully
            std::cout << "Client disconnected." << std::endl;
        }
        else
            std::cerr << "Exception: " << e.what() << std::endl;
        break;
    }
}
}

int main() {
if (!fs::exists(SERVER_DIR)) {
    fs::create_directory(SERVER_DIR);
    std::cout << "Created server directory: " << SERVER_DIR << std::endl;
}

try {
    // Create an io_context object
    boost::asio::io_context io_context;

    // Create a TCP acceptor listening on the specified port
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), PORT));

    std::cout << "Server is listening on port " << PORT << std::endl;

    // Wait for a client to connect
    tcp::socket socket(io_context);
    acceptor.accept(socket);
    std::cout << "Client connected!" << std::endl;
    //std::ofstream output_file;



    for (;;) {
        recive_files(socket);
        printf("here\n");
    }

        
        
}
catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
}

return 0;
}
