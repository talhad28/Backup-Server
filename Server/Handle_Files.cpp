#include "Handle_Files.hpp"
#include "Protocol.hpp"




void File_transfer::send_file(tcp::socket& socket, const std::string& user_dir, const std::string& file_name) {
    std::string full_file_path = user_dir + "\\" + file_name;
    size_t totalSent = 0;
    Server_Protocol prot;
    try {
        std::ifstream file(full_file_path, std::ios::binary);
        if (!file)
            prot.status = status_code(ERR_NOT_EXIST);
        else {
            prot.status = status_code(FILE_RESTORE_OK);
            //get file size
            file.seekg(0, std::ios::end);
            std::streamsize file_size = file.tellg();
            file.seekg(0, std::ios::beg);

            // Check for overflow
            if (file_size > std::numeric_limits<uint32_t>::max()) {
                // Handle error - file is too large
                prot.status = status_code(ERR_GENERAL);
            }
            prot.payload.payload_size = static_cast<uint32_t>(file_size);
            prot.file_name = file_name;
            prot.name_len = static_cast<uint16_t>(file_name.length());

            //check if we can send all in one package. only if the file file_size(4)+version(1)+status(2)+name_len(2)+file_name(name_len)+file <= 1MB.
            if (file_size < PACKET_SIZE - file_name.length() - 9) { 
                std::vector<uint8_t> buffer(file_size);
                file.read(reinterpret_cast<char*>(buffer.data()),file_size);
                prot.payload.payload = buffer;
                std::vector<uint8_t> packet = prot.serialize();
                uint64_t packet_size = packet.size();
                boost::asio::write(socket, boost::asio::buffer(&packet_size, sizeof(packet_size)));
                boost::asio::write(socket, boost::asio::buffer(packet, packet_size));
            } 

            //file size to big so we need to send it in chunks
            else {
                std::vector<uint8_t> buffer(PACKET_SIZE - file_name.length() - 9);
                file.read(reinterpret_cast<char*>(buffer.data()), PACKET_SIZE - file_name.length() - 9);
                prot.payload.payload = buffer;
                std::vector<uint8_t> packet = prot.serialize();
                uint64_t packet_size = packet.size();
                boost::asio::write(socket, boost::asio::buffer(&packet_size, sizeof(packet_size)));
                boost::asio::write(socket, boost::asio::buffer(packet, packet_size));
                totalSent += PACKET_SIZE - file_name.length() - 9;
                prot.payload.payload.resize(PACKET_SIZE);
                while (totalSent < file_size) {
                    size_t remaining = file_size - totalSent;
                    size_t chunk = std::min(PACKET_SIZE, remaining);
                    buffer.resize(chunk);
                    file.read(reinterpret_cast<char*>(buffer.data()), chunk);
                    boost::asio::write(socket, boost::asio::buffer(buffer.data(), chunk));
                    totalSent += chunk;
                }
            }
            std::cout << "File sent!" << std::endl;
        }
        get_ok_msg(socket);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

}

 void File_transfer::save_file(tcp::socket& socket, const std::string& full_file_path, size_t payload_size) {
     Server_Protocol prot;
     std::ofstream output_file(full_file_path, std::ios::binary);
     if (!output_file.is_open()) {
         throw std::runtime_error("Failed to open file for writing: " + full_file_path);
     }

     size_t total_bytes_received = 0;
     std::vector<char> buffer(PACKET_SIZE);

     while (total_bytes_received < payload_size) {
         // Calculate how many bytes to read in this iteration
         size_t bytes_to_read = std::min(PACKET_SIZE, payload_size - total_bytes_received);

         // Read the chunk from the socket
         size_t bytes_read = boost::asio::read(socket, boost::asio::buffer(buffer, bytes_to_read));
         total_bytes_received += bytes_read;

         // Write the chunk to the file
         output_file.write(buffer.data(), bytes_read);
         std::cout << "Received " << total_bytes_received << "/" << payload_size << " bytes" << std::endl;
     }
	 std::cout << "File received!" << std::endl;

     output_file.close();
     prot.status = status_code(FILE_BACKUP_OK);
     std::vector<uint8_t> packet = prot.serialize();
     uint64_t packet_size = packet.size();
     boost::asio::write(socket, boost::asio::buffer(&packet_size, sizeof(packet_size)));
     boost::asio::write(socket, boost::asio::buffer(packet, packet_size));
     get_ok_msg(socket);
 }

 void File_transfer::return_file_list(tcp::socket& socket, const std::string& user_dir) {
    Server_Protocol prot;
    std::string files = "";
    for (const auto& entry : fs::directory_iterator(user_dir)) {
                files += "\n" + entry.path().filename().string();
    }
    prot.set_payload(files);
    prot.status = files.empty() ? 1002 : 211;
    prot.name_len = 0;
    std::vector<uint8_t> packet = prot.serialize();

    uint64_t packet_size = packet.size();
    boost::asio::write(socket, boost::asio::buffer(&packet_size, sizeof(packet_size)));

    boost::asio::write(socket, boost::asio::buffer(packet, packet_size));
    get_ok_msg(socket);
 }
 
 void File_transfer::delete_file(tcp::socket& socket, const std::string& full_file_path) {
     Server_Protocol prot;
	 if (fs::remove(full_file_path)) {
		 std::cout << "File deleted: " << full_file_path << std::endl;
         prot.status = status_code(FILE_BACKUP_OK);
	 }
	 else {
		 std::cout << "Failed to delete file: " << full_file_path << std::endl;
         prot.status = status_code(ERR_NOT_EXIST);
	 }
     std::vector<uint8_t> packet = prot.serialize();
     uint64_t packet_size = packet.size();
     boost::asio::write(socket, boost::asio::buffer(&packet_size, sizeof(packet_size)));
     boost::asio::write(socket, boost::asio::buffer(packet, packet_size));
     get_ok_msg(socket);
 }

  