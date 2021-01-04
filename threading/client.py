#!/usr/bin/python3
import socket
import sys
import os
import argparse
import signal
import struct

'''
global file_list variable meant to store directory contents
from server used in get_file_list and recv_file_list
'''
file_list = []


'''
obtain command line arguments of the IP and port and return
the data to obtain the network socket information
'''
def socket_info():
    parser = argparse.ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("port", type=int)
    args = parser.parse_args()
    host = args.host
    port = args.port
    return host, port

'''
catch SIGINT signal to exit server gracefully if user does not
enter proper exit menu option
'''
def signal_handler(sig, frame):
    command = 500
    exit = struct.pack("I", command)
    sent = cli_socket.send(exit)
    goodbye = "exit"
    totalsent = 0
    while totalsent < len(goodbye):
        sent = cli_socket.send(goodbye[totalsent:].encode('utf-8'))
        if sent == 0:
            raise RuntimeError("Socket connection broken: sending exit")
        totalsent += sent
    print("\nCtrl+C caught and handled... Goodbye!")
    cli_socket.close()
    sys.exit(0)

'''
take the return values of socket_info() and create the network
connection to the server
'''
cli_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_addr, port = socket_info()
try:
    cli_socket.connect((server_addr, port))
    print("Connected to {}:{}".format(server_addr, port))
except OSError as msg:
    print("Closing socket on failure")
    cli_socket.close()
    cli_socket = None
    exit()

'''
create a main menu method that gives the user the option to upload
files within their own directories or download files existing within
the file server
'''
def main_menu():
    print("\n#--------------------------------------------------#")
    print("#-------------------FILE SERVER--------------------#")
    print("#--------------------------------------------------#")
    print("Please enter an option from the following menu:")
    print("1. Download Existing File from Server")
    print("2. Upload User File to Server")
    print("3. View Files Currently on the File Server")
    print("4. File Server Help")
    print("5. Exit File Server")
    menu_option = input("/> ")
    menu_option.rstrip()
    try:
        in_type = int(menu_option)
        if in_type <= 5 or in_type >= 1:
            return in_type
    except ValueError:
        print("Invalid menu option selected... Breaking connection")
        exit()
    

'''
help menu option method:
explains how to view File Server Directory Contents
explains how to download a file from the file server
explains how to upload a file from your workstation to the file server
'''
# come back to this after setting up improved functionality
def help_option(menu_option):
    if menu_option == 4:
        print("\n#--------------------------------------------------#")
        print("#--------------------HELP MENU---------------------#")
        print("#--------------------------------------------------#")
        print("Menu option 1 allows the user download files from the file server")
        print("Menu option 2 allows the user to upload files to the file server")
        print("Menu option 3 displays all downloadable files within the server")
        print("Menu option 5 allows the user to exit the file server program")
        print("If necessary, the user can enter 'ctrl+c' to gracefully exit the program")     

'''
simple exit command for quitting file server
'''
def exit_server(menu_option):
    if menu_option == 5:
        command = 500
        exit = struct.pack("I", command)
        sent = cli_socket.send(exit)
        goodbye = "exit"
        totalsent = 0
        while totalsent < len(goodbye):
            sent = cli_socket.send(goodbye[totalsent:].encode('utf-8'))
            if sent == 0:
                raise RuntimeError("Socket connection broken: sending exit")
            totalsent += sent
            print(goodbye)
        print("Thank you.... Goodbye!")
        cli_socket.close()

'''
obtain the file size requested for download from the server
used to make sure the appropriate amount of data is downloaded
from the server in order to ensure proper file transfer
'''
def get_file_size():
    data = cli_socket.recv(8)
    file_size = int.from_bytes(data, byteorder='little', signed=True)
    return file_size

'''
perform avalidation check to determine if the client specified directory
and file exists within the client machine
'''
def is_client_file(dir, file):
    if os.path.isdir(dir):
        path = dir + file
        if os.path.isfile(path):
            print("OK")
            return True
        else:
            return False
    else:
        print("No such file or directory")

        return False

'''
create a new file after calling download functionality
allows the user to create a file within the "Client" directory
saves the downloaded contents to the newly created file
'''
def download_file():
    requested_file_sz = get_file_size()
    if requested_file_sz == -1:
        print("Requested file not found on server...")
        return None
    newfile = input("Enter the name you wish to store your file as: ")
    newfile = newfile.rstrip()
    download_path = "Client/" + newfile
    bytes_left = requested_file_sz
    print("The requested file is {} bytes in length".format(requested_file_sz))
    try:
        with open(download_path, 'wb+') as file_ptr:
            while bytes_left > 0:
                contents = cli_socket.recv(1024)
                if contents is None:
                    raise RuntimeError("Socket connection broken: download")
                file_ptr.write(contents)
                bytes_left -= 1024
        print("Total bytes downloaded: {}".format(requested_file_sz))
        print("DONE")
    except OSError:
        print("Could not download file from server")


'''
allow the user to select a file from the list received from the
server, and give the user the option to read the contents
or download the file
'''
def download_existing_file(menu_option):
    if menu_option == 1:
        command = 200
        d_load = struct.pack("I", command)
        sent = cli_socket.send(d_load)
        if sent == 0:
            raise RuntimeError("Socket connection broken: send download command")
        choice = input("Type the name of the file you wish to download: ")
        choice = choice.rstrip()
        totalsent = 0
        while totalsent < len(choice):
            sent = cli_socket.send(choice[totalsent:].encode('utf-8'))
            if sent == 0:
                raise RuntimeError("Socket connection broken: choose action - file")
            totalsent += sent
        print("You chose to download {} from the server".format(choice))
        download_file()
    else:
        raise RuntimeError("Invalid menu option received: download existing")

'''
call validation check on user_in and prepare and send file
for upload to File Server
'''
def upload_file(menu_option):
    if menu_option == 2:
        command = 300
        upload = struct.pack("I", command)
        sent = cli_socket.send(upload)
        if sent == 0:
            raise RuntimeError("Socket connection broken: upload command")
        totalsent = 0
        send_file = "upload"
        try:
            while totalsent < len(send_file):
                sent = cli_socket.send(send_file[totalsent:].encode('utf-8'))
                if sent == 0:
                    raise RuntimeError("Socket connection broken: send upload request")
                totalsent += sent
                print("Sent to upload request to server")
        except OSError:
            raise RuntimeError("Could not send upload request command")
        user_in = input("Enter the directory location of your file: ")
        user_in = user_in.rstrip()
        dir = "/home/jes/Documents/Practice/threading/" + user_in + "/"
        file = input("Enter the file you wish to upload: hint - include extension\n")
        file = file.rstrip()
        if is_client_file(dir, file) is True:
            path = dir + file
            file_size = os.path.getsize(path)
            size = struct.pack("I", file_size)
            sent = cli_socket.send(size)
            if sent == 0:
                raise RuntimeError("Socket connection broken: upload file size")
            name_len = len(file)
            n_len = struct.pack("I", name_len)
            sent = cli_socket.send(n_len)
            if sent == 0:
                raise RuntimeError("Socket connection broken: file name size")
            totalsent = 0
            while totalsent < len(file):
                sent = cli_socket.send(file[totalsent:].encode('utf-8'))
                if sent == 0:
                    raise RuntimeError("Socket connection broken: file name")
                totalsent += sent
            totalsent = 0
            print("Sending {} to File Server".format(path))
            with open(path, "br") as file_ptr:
                while totalsent < file_size:
                    contents = file_ptr.read(1024)
                    sent = cli_socket.send(contents)
                    if sent == 0:
                        raise RuntimeError("Socket connection broken: sending contents")
                    totalsent += sent
            print("Upload Complete")
        else:
            err_code = 666
            size = struct.pack("I", err_code)
            sent = cli_socket.send(size)
            if sent == 0:
                raise RuntimeError("Socket connection broken: invalid upload file")
    else:
        raise RuntimeError("Invalid menu option received: upload file")

'''
get the length of the file name
used as a helper method for the get file lis
this will allow the directory list call to always grab the
file listing on the server
'''
def recv_file_list():
    file_count = cli_socket.recv(4)
    file_count = int.from_bytes(file_count, "little", signed=True)
    received = 0
    total_recv = 0
    data = b''
    #add second recv call to get file name and append to list
    while received < file_count:
        file_len = cli_socket.recv(8)
        file_len = int.from_bytes(file_len, "little", signed=True)
        data = cli_socket.recv(file_len)
        file_list.append(data.decode('utf-8'))
        received += 1
        total_recv += file_len
    print(file_list)

'''
takes the user's input from the main menu,
'''
def get_file_list(menu_option):
    if menu_option == 3:
        request = 100
        list_dir = struct.pack("I", request)
        sent = cli_socket.send(list_dir)
        if sent == 0:
            raise RuntimeError("Socket connection broken: get dir list")
        print("Sent list directory command to server...")
        totalsent = 0
        list_cmd = "ls"
        while totalsent < len(list_cmd):
            sent = cli_socket.send(list_cmd[totalsent:].encode('utf-8'))
            if sent == 0:
                raise RuntimeError("Socket connection broken: sending list command")
            totalsent += sent
        recv_file_list()
    else:
        raise RuntimeError("Invalid menu option received: get file")
    file_list.clear()

    
if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    while True:
        user_in = main_menu()
        if user_in == 1:
            download_existing_file(user_in)
        elif user_in == 2:
            upload_file(user_in)
        elif user_in == 3:
            get_file_list(user_in)
        elif user_in == 4:
            help_option(user_in)
        elif user_in == 5:
            exit_server(user_in)
            exit()
        elif user_in > 5 or user_in <= 0:
            print("Please select an appropriate menu option...")
