import socket
import selectors
import random
import struct
import os
from enum import Enum
from turtle import back

MAX_NAME_LECGTH = 256
PACKET_SIZE = 1024 * 1024

class OP(Enum):  # Request codes
        FILE_BACKUP = 100  # Save file backup. All fields should be valid.
        FILE_RESTORE = 200  # Restore a file. size, payload unused.
        FILE_DELETE = 201  # Delete a file. size, payload unused.
        FILE_DIR = 202  # List all client's files. name_len, filename, size, payload unused.

class Payload:
    def __init__(self):
        self.size = 0
        self.payload = b""

class Protocol:
    def __init__(self, ID, ver):
        self.user_id = ID
        self.op = 0
        self.version = ver
        self.name_len = 0
        self.file_name = b""
        self.payload = Payload()

    def set_file(self, filename):
        self.name_len = len(filename)
        self.set_file_name(filename)
        self.payload.size = os.path.getsize(filename)
        with open(filename, "rb") as f:
            file_data = f.read()
            self.payload.payload = file_data
        print(f"Sending file '{filename}' with size: {self.payload.size}")

    def set_file_name(self, filename):
        self.file_name = bytes(filename, 'utf-8')
        self.name_len = len(filename)

    def set_op(self, op):
        self.op = op

    def serialize(self):
        # Pack the protocol fields into a binary format
        header = struct.pack(
            "<I B B H I",  # Format: user_id (4 bytes), version (1 byte), op (1 byte), name_len (2 bytes), paylaod size (4 bytes)
            self.user_id,
            self.version,
            self.op,
            self.name_len,
            self.payload.size
        )
        return header


def backup_request(file, sock):
    prot = Protocol(user_ID, VERSION)
    prot.set_file(file)
    prot.set_op(OP.FILE_BACKUP.value)  # Set the operation code for backup

    # Serialize the protocol and send it to the server

    #send header
    sock.sendall(prot.serialize())

    #send file name
    sock.sendall(prot.file_name)
    print(f"Sent protocol header and file name: {prot.file_name}")

    send_file_in_chunks(sock, file)
    sock.close()
    print(f"Sent protocol for file: {file}")

def restore_request(file, sock):
    prot = Protocol(user_ID, VERSION)
    prot.set_op(OP.FILE_RESTORE.value)
    prot.set_file_name(file)

    sock.sendall(prot.serialize())
    sock.sendall(prot.file_name)

    file_size_bytes = sock.recv(8)
    file_size = int.from_bytes(file_size_bytes, 'little')
    print(f"Receiving a file of size {file_size} bytes")

    with open(file, "wb") as output_file:
        total_bytes_recived = 0
        while total_bytes_recived < file_size:
            data = sock.recv(min(PACKET_SIZE, file_size - total_bytes_recived))
            if not data:
                break
            output_file.write(data)
            total_bytes_recived += len(data)
   
        


def send_file_in_chunks(sock, filename):
    file_size = os.path.getsize(filename)
    print(f"Sending file '{filename}' of size {file_size} bytes")

    with open(filename, "rb") as f:
        bytes_sent = 0
        while bytes_sent < file_size:
            chunk = f.read(1024 * 1024)  # Read 1 MB at a time
            if not chunk:
                break
            sock.sendall(chunk)
            bytes_sent += len(chunk)
            print(f"Sent {bytes_sent}/{file_size} bytes")

    #def list_request():

    #def del_request(name):

if __name__ == "__main__":
    host = '127.0.0.1'
    port = 1234
    #user_ID = random.randint(1000, 9999)
    user_ID = 1232
    VERSION = 1
    CHUNK_SIZE = 1024
    #f1 = open("server.info")
    back_info_file = open("backup.info")
    #server_info = f1.read()
    backup_info = [f for f in back_info_file.readlines()] #read files name to array
    backup_info = [f.replace("\n","") for f in backup_info] #removes '\n' from files' name
    #f1.close()
    back_info_file.close()
    with socket.create_connection((host,port)) as sock:
         for file in backup_info:       
             restore_request(file, sock)
             print("finished")
             #sock.send(("test2").encode("utf-8"))
        
    # test = Protocol(1,1)
    # test.set_file(backup_info[0])
    # print(test)
    # prot = Protocol(1234, 1)  # Example user_id = 1234
    # prot.set_file(file)
    # serialized_data = prot.serialize()
    # print(serialized_data[:4])  # Inspect the first 4 bytes for user_id

    



