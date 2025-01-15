import socket
import selectors
import random
import struct
from enum import Enum

MAX_NAME_LECGTH = 256

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
        self.payload = b""

    def set_file(self, filename):
        self.name_len = len(filename)
        self.file_name = bytes(filename, 'utf-8')
        with open(filename, "rb") as f:
            self.payload = f.read()

    def set_op(self, op):
        self.op = op

    def serialize(self):
        # Pack the protocol fields into a binary format
        header = struct.pack(
            "<I B B H",  # Format: user_id (4 bytes), version (1 byte), op (1 byte), name_len (2 bytes)
            self.user_id,
            self.version,
            self.op,
            self.name_len,
        )
        # Append the file name and payload
        return header + self.file_name + self.payload


def backup_request(file):
    prot = Protocol(user_ID, VERSION)
    prot.set_file(file)
    prot.set_op(200)  # Set the operation code for backup

    # Serialize the protocol and send it to the server
    sock.sendall(prot.serialize())
    print(f"Sent protocol for file: {file}")





# def backup_request(file):
#     prot = protocol(user_ID)
#     prot.set_file(file)
#     #sending the file's name and the file's name length
#     #file_name_bytes = file.encode('utf-8')
#     #file_name_length = len(file_name_bytes).to_bytes(4, byteorder='big')  # 4-byte length
#     sock.send(prot)
#     with open(file, "rb") as f:
#         while chunk := f.read(CHUNK_SIZE):
#             sock.send(chunk)  # Send each chunk to the server

#             # Wait for acknowledgment from the server (e.g., "Ready")
#             response = sock.recv(1024)
#             #print(f"Received from server: {response.decode('utf-8')}")
#     sock.send(b"EOF")

    #def restore_request(name):

    #def list_request():

    #def del_request(name):




if __name__ == "__main__":
    host = '127.0.0.1'
    port = 1234
    #user_ID = random.randint(1000, 9999)
    user_ID = 1234
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
             backup_request(file)
             print("finished")
             sock.send(("test2").encode("utf-8"))
        
    # test = Protocol(1,1)
    # test.set_file(backup_info[0])
    # print(test)
    # prot = Protocol(1234, 1)  # Example user_id = 1234
    # prot.set_file(file)
    # serialized_data = prot.serialize()
    # print(serialized_data[:4])  # Inspect the first 4 bytes for user_id

    



