#include "Handle_Files.hpp"

 const size_t CHUNK_SIZE = 1024 * 1024; // 1 MB per chunk


void receive_file_in_chunks(tcp::socket& socket, const std::string& file_name, size_t payload_size) {
    std::ofstream output_file(file_name, std::ios::binary);
    if (!output_file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + file_name);
    }

    size_t total_bytes_received = 0;
    std::vector<char> buffer(CHUNK_SIZE);

    while (total_bytes_received < payload_size) {
        // Calculate how many bytes to read in this iteration
        size_t bytes_to_read = std::min(CHUNK_SIZE, payload_size - total_bytes_received);

        // Read the chunk from the socket
        size_t bytes_read = boost::asio::read(socket, boost::asio::buffer(buffer, bytes_to_read));
        total_bytes_received += bytes_read;

        // Write the chunk to the file
        output_file.write(buffer.data(), bytes_read);
        std::cout << "Received " << total_bytes_received << "/" << payload_size << " bytes" << std::endl;
    }

    output_file.close();
}

static void send_files_in_chunks(tcp::socket& socket, const std::string& file_name) {
    try {
        std::ifstream file(file_name, std::ios::binary);
        if (!file)
            throw std::runtime_error("could not open file: " + file_name);

        //get file size
        file.seekg(0, std::ios::end);
        size_t payload_size = file.tellg();
        file.seekg(0, std::ios::beg);

        //send the file size
        boost::asio::write(socket, boost::asio::buffer(&payload_size, sizeof(payload_size)));

        //crate buffer for chunks
        std::vector<char> buffer(CHUNK_SIZE);
        size_t totalSent = 0;
        while (totalSent < payload_size) {
            size_t remaining = payload_size - totalSent;
            size_t chunk = std::min(CHUNK_SIZE, remaining);
            file.read(buffer.data(), chunk);
            boost::asio::write(socket, boost::asio::buffer(buffer.data(), chunk));
            totalSent += chunk;
        }
        std::cout << "File sent!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
}

void save_file(tcp::socket& socket, const std::string& file_name, std::string& user_dir, size_t payload_size) {
    // Save the file in the user directory
    std::string full_file_path = user_dir + "\\" + file_name;
    std::ofstream output_file(full_file_path, std::ios::binary);
    if (output_file.is_open()) {
        receive_file_in_chunks(socket, full_file_path, payload_size);
        output_file.close();
        std::cout << "File saved to: " << full_file_path << std::endl;
    }
    else {
        std::cerr << "Failed to create file: " << full_file_path << std::endl;
    }
}

void send_file(tcp::socket& socket, const std::string& file_name, std::string& user_dir) {
    std::string full_file_path = user_dir + "\\" + file_name;
    send_files_in_chunks(socket, full_file_path);
}
