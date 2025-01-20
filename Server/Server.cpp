#pragma once
#include "Handle_Files.hpp"
#include "Protocol.hpp"

void session(tcp::socket socket) {
    try {
        std::cout << "Client connected!" << std::endl;
        handle_client_protocol(socket, parse_client_protocol(socket));
        socket.close();
    }
    catch (std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << std::endl;
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

        for (;;) {
            std::thread(session, acceptor.accept()).detach();
        }
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
