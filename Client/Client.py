import socket
import random
import struct
import os
from enum import Enum
from turtle import back

MAX_NAME_LENGTH = 256
PACKET_SIZE = 1024 * 1024 #max packet size 1MB
OK_MSG = bytearray("OK\0", 'utf-8')

class OP(Enum):  # Request codes
        FILE_BACKUP = 100  # Save file backup. All fields should be valid.
        FILE_RESTORE = 200  # Restore a file. size, payload unused.
        FILE_DELETE = 201  # Delete a file. size, payload unused.
        FILE_DIR = 202  # List all client's files. name_len, filename, size, payload unused.

class Status(Enum):  # Response codes
    FILE_RESTORE_OK = 210  # File restore successful. size, payload fields unused.
    FILE_DIR_OK = 211  # File list successful, all fields used.
    FILE_BACKUP_OK = 212 # File backup successful/File delete successful. size, payload fields unused
    ERR_NOT_EXIST = 1001  # File does not exist. size, payload fields unused.
    ERR_NO_FILES = 1002   # No files to list. size, payload fields unused.
    ERR_GENERAL = 1003    # General error. only version and staues fields are used

class Payload:
    def __init__(self):
        self.size = 0
        self.payload = b""

class Client_Protocol:
    def __init__(self, ID, ver):
        self.user_id = ID
        self.op = 0
        self.version = ver
        self.name_len = 0
        self.file_name = b""
        self.payload = Payload()

    def set_file(self, file):
        file_data = file.read(PACKET_SIZE - self.name_len - 12) # Read the remaining space in the packet (out of 1MB)
        self.payload.payload = file_data

    def set_name_len(self, name_len):
        if name_len > MAX_NAME_LENGTH:
            raise ValueError("Name length is too long")
        self.name_len = name_len

    def set_file_name(self, filename):
        self.file_name = bytes(filename, 'utf-8')
        self.name_len = len(filename)

    def set_op(self, op):
        self.op = op

    def set_payload_size(self, size):
        self.payload.size = size

    # Pack the protocol fields into a binary format
    def serialize(self):
        # Format: user_id (4 bytes), version (1 byte), op (1 byte), name_len (2 bytes), paylaod size (4 bytes as long), name (name_len bytes), payload (the remaining space out of 1MB, or payload_size. the smaller of the two)

        format_string = f"<I B B H {self.name_len}s L {min(PACKET_SIZE - self.name_len - 12, self.payload.size)}s"
        packet = struct.pack(format_string, 
            #header
            self.user_id,
            self.version,
            self.op,
            self.name_len,
            self.file_name,
            #payload
            self.payload.size,
            self.payload.payload
        )
        return packet

    def get_header_size(self):
        return 12 + self.name_len # 12 bytes for the header + name_len bytes for the file name

class Server_protocol:
    def __init__(self):
        self.version = 0
        self.status = 0
        self.name_len = 0
        self.file_name = b""
        self.payload = Payload()

    def deserialize(self, data):
        # First unpack the fixed header (version, status, name_len)
        header = struct.unpack_from('<BHH', data, 0)
        self.version = data[0]
        self.status = header[1]
        self.name_len = header[2]
        print(f"Version: {self.version}, Status: {self.status}, Name length: {self.name_len}")
        # Read name_len (2 bytes, little-endian)
        # self.name_len = struct.unpack_from('<H', data, 2)[0]
    
        filename_start = 5  # After version(1) + status(2) + name_len(2)
        filename_end = filename_start + self.name_len
        self.file_name = struct.unpack_from(f'<{self.name_len}s', data, filename_start)[0];
        print(f"Filename: {self.file_name}")
        
        # Read payload size
        payload_size_start = filename_end
        self.payload.size = struct.unpack_from('<L', data, payload_size_start)[0]
        
        # Read payload data
        payload_start = payload_size_start + 4
        payload_end = payload_start + self.payload.size
        self.payload.payload = data[payload_start:payload_end]

def parse_protocol(packet):
    prot = Server_protocol()
    prot.deserialize(packet)
    return prot

def send_file_in_chunks(sock, file, file_size):
    bytes_sent = 0
    while bytes_sent < file_size:
        chunk = file.read(PACKET_SIZE)  # Read 1 MB at a time
        if not chunk:
            break
        sock.sendall(chunk)
        bytes_sent += len(chunk)
        print(f"Sent {bytes_sent}/{file_size} bytes")

def backup_request(file, host, port):
    with socket.create_connection((host,port)) as sock:
        prot = Client_Protocol(user_ID, VERSION)
        prot.set_op(OP.FILE_BACKUP.value)  # Set the operation code for backup
        prot.payload.size = os.path.getsize(file)
        prot.set_file_name(file)
        prot.set_name_len(len(file))
        print(f"Sending file '{file}' with size: {prot.payload.size}")
        with open(file, "rb") as f:
            prot.set_file(f)
            sock.sendall(prot.serialize())

            # if file too big send rest in chunks
            if prot.get_header_size() + prot.payload.size > PACKET_SIZE:
                send_file_in_chunks(sock, f, prot.payload.size)
        response_size = sock.recv(8)
        total_size = int.from_bytes(response_size, byteorder='little')
        response = sock.recv(total_size)
        server_prot = parse_protocol(response)
        if server_prot.status == Status.FILE_BACKUP_OK.value:
            print("Backup successful")
        elif server_prot.status == Status.ERR_GENERAL.value:
            print("Backup failed: General error")
        sock.sendall(OK_MSG)

def restore_request(file, host, port):
    with socket.create_connection((host,port)) as sock:
        prot = Client_Protocol(user_ID, VERSION)
        prot.set_op(OP.FILE_RESTORE.value)
        prot.set_file_name(file)
        sock.sendall(prot.serialize())
        response_size = sock.recv(8)
        total_size = int.from_bytes(response_size, 'little')
        response = sock.recv(total_size)
        server_prot = parse_protocol(response)
        if server_prot.status == Status.FILE_RESTORE_OK.value:
            with open(file, "wb") as output_file:
                output_file.write(server_prot.payload.payload)

                #check if we need to get more packets, and handle them
                if server_prot.payload.size > PACKET_SIZE - server_prot.name_len - 9:
                    total_bytes_recived = PACKET_SIZE - server_prot.name_len - 9 # what that was sent in the first packet
                    while total_bytes_recived < server_prot.payload.size:
                        data = sock.recv(min(PACKET_SIZE, server_prot.payload.size - total_bytes_recived))
                        if not data:
                            break
                        output_file.write(data)
                        total_bytes_recived += len(data)
            if os.path.getsize(file) == server_prot.payload.size:
                print("File restored successfully")
            else:
                print("File restoration failed: File size mismatch, please try again")
        elif server_prot.status == Status.ERR_NOT_EXIST.value:
            print("File restoration failed: File does not exist on server")
        elif server_prot.status == Status.ERR_GENERAL.value:
            print("File restoration failed: General error")
        sock.sendall(OK_MSG)
   
def lst_request(host, port):
    with socket.create_connection((host,port)) as sock:
        prot = Client_Protocol(user_ID, VERSION)
        prot.set_op(OP.FILE_DIR.value)

        sock.sendall(prot.serialize())

        size_data = sock.recv(8)
        total_size = int.from_bytes(size_data, byteorder='little')

        # Receive the list of files
        data = sock.recv(total_size)

        server_prot = parse_protocol(data);
    
        # Decode and split into list of filenames
        files_list = server_prot.payload.payload.decode('utf-8').strip().split('\n')
        print(f"Received files: {files_list}")
    
        # Send acknowledgment after receiving the data
        sock.sendall(OK_MSG)

def del_request(file, host, port):
    with socket.create_connection((host,port)) as sock:
        prot = Client_Protocol(user_ID, VERSION)
        prot.set_op(OP.FILE_DELETE.value)
        prot.set_file_name(file)
        sock.sendall(prot.serialize())
        data_size = sock.recv(8)
        total_size = int.from_bytes(data_size, byteorder='little')
        response = sock.recv(total_size)
        server_prot = parse_protocol(response)
        if server_prot.status == Status.FILE_BACKUP_OK.value:
            print("File deleted successfully")
        elif server_prot.status == Status.ERR_NOT_EXIST.value:
            print("File deletion failed: File does not exist on server")
        elif server_prot.status == Status.ERR_GENERAL.value:
            print("File deletion failed: General error")
        sock.sendall(OK_MSG)

def get_srv_info():
    with open("server.info") as file:
        server_info = file.read()
    host, port = server_info.split(":")
    return host, int(port)

def get_backup_info():
    with open("backup.info") as file:
        backup_info = [f for f in file.readlines()] #read files name to array
    return [f.replace("\n","") for f in backup_info] #removes '\n' from files' name

if __name__ == "__main__":
    user_ID = random.randint(1000, 9999)
    VERSION = 1
    host, port = get_srv_info()
    backup_info = get_backup_info()
    lst_request(host, port)
    backup_request(backup_info[0], host, port)
    backup_request(backup_info[1], host, port)
    restore_request(backup_info[0], host, port)
    restore_request(backup_info[1], host, port)
    del_request(backup_info[0], host, port)
    del_request(backup_info[1], host, port)
    

    



