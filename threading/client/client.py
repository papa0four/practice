#!/usr/bin/python3
import socket
import sys
import os
import argparse
import signal
import struct

'''
GLOBAL VARIABLE: FILE_LIST
    TYPE: list()
    BRIEF: stores list of files that exists on file server
    USAGE: used within the RECV_FILE_LIST and GET_FILE_LIST methods
'''
file_list = []

def socket_info():
    '''
    SOCKET_INFO:
        PARAMETER: None
        BRIEF: parses cmdline arguments to get ip address and port info
        RETURN: server ip address and socket port
    '''
    parser = argparse.ArgumentParser()
    parser.add_argument("host")
    parser.add_argument("port", type=int)
    args = parser.parse_args()
    host = args.host
    port = args.port
    return host, port

def signal_handler(sig, frame):
    '''
    SIGNAL_HANDLER:
        PARAMETER: SIG - signal to be caught
        PARAMETER: FRAME - stack frame pointer during the signal interruption
        BRIEF: catches the ctrl+c SIGINT signal from the user, 
                sends exit command to server
        RETURN: None
    '''
    command = -1
    exit = struct.pack("i", command)
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
GLOBAL: CLI_SOCKET, SERVER_ADDR, PORT
    BRIEF: creates the socket connection to the server
    USAGE: calls the socket_info method and creates the socket,
            exits program on failure
'''
cli_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_addr, port = socket_info()
try:
    cli_socket.connect((server_addr, port))
    print(f"Connected to {server_addr}:{port}")
except OSError as msg:
    print("Closing socket on failure")
    cli_socket.close()
    cli_socket = None
    exit()

def main_menu():
    '''
    MAIN_MENU:
        PARAMETER: None
        BRIEF: creates and displays a menu for the user to interact with
                only allows for int value input (1 - 5)
        RETURN: returns menu option selected on success, exits client program
                on invalid menu option
    '''
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
    
def help_option(menu_option):
    '''
    HELP_OPTION:
        PARAMETER: MENU_OPTION - int value of 4
        BRIEF: prints a brief synopsis of how to use the client application
        RETURN: None
    '''
    if menu_option == 4:
        print("\n#--------------------------------------------------#")
        print("#--------------------HELP MENU---------------------#")
        print("#--------------------------------------------------#")
        print("Menu option 1 allows the user download files from the file server")
        print("Menu option 2 allows the user to upload files to the file server")
        print("Menu option 3 displays all downloadable files within the server")
        print("Menu option 5 allows the user to exit the file server program")
        print("If necessary, the user can enter 'ctrl+c' to gracefully exit the program")     

def exit_server(menu_option):
    '''
    EXIT_SERVER:
        PARAMETER: MENU_OPTION - int value of 5
        BRIEF: allows for a graceful client connection closure
        RETURN: None
    '''
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
        print("Thank you.... Goodbye!")
        cli_socket.close()

def get_file_size():
    '''
    GET_FILE_SIZE:
        PARAMETER: None
        BRIEF: gets the size of the file to be used in the download_file method
        RETURN: FILE_SIZE - int value size of file to be downloaded
    '''
    data = cli_socket.recv(8)
    file_size = int.from_bytes(data, byteorder='big', signed=True)
    return file_size

def is_client_file(dir, file):
    '''
    IS_CLIENT_FILE:
        PARAMETER: DIR - directory to check for valid client file
        PARAMETER: FILE - file to check for validity
        BRIEF: checks the validity of a directory and file passed by the user
                meant to be uploaded to the server
        RETURN: (bool) true on success (file/directory), false on failures
    '''
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

def download_file():
    '''
    DOWNLOAD_FILE:
        PARAMETER: None
        BRIEF: allows the client to download a specified file from the server
        RETURN: None, returns None on failure for execution
    '''
    requested_file_sz = get_file_size()
    if requested_file_sz == -1:
        print("Requested file not found on server...")
        return None
    newfile = input("Enter the name you wish to store your file as: ")
    newfile = newfile.rstrip()
    download_path = "../ClientDir/" + newfile
    bytes_left = requested_file_sz
    print(f"The requested file is {requested_file_sz} bytes in length")

    try:
        with open(download_path, 'wb+') as file_ptr:
            if os.path.isfile(download_path):
                file_ptr.seek(0)

            while bytes_left > 0:
                contents = cli_socket.recv(1024)
                if contents is None:
                    raise RuntimeError("Socket connection broken: download")
                file_ptr.write(contents)
                bytes_left -= 1024
        print("Total bytes downloaded: {}".format(requested_file_sz))
        print("DONE")
    except OSError as file_err:
        print(f"Could not download file from server: {file_err}")

def download_existing_file(menu_option):
    '''
    DOWNLOAD_EXISTING_FILE:
        PARAMETER: MENU_OPTION - menu option 1
        BRIEF: sends download command to server and allows the user to obtain
                a file located on the server
        RETURN: None
    '''
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

def upload_file(menu_option):
    '''
    UPLOAD_FILE:
        PARAMETER: MENU_OPTION - menu option 2 for file upload
        BRIEF: allows the client to specify a file to send to the server
        RETURN: None
    '''
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
        cwd = os.getcwd()
        dir = "../" + user_in + "/"
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
            err_code = -1
            size = struct.pack("i", err_code)
            sent = cli_socket.send(size)

            if sent == 0:
                raise RuntimeError("Socket connection broken: invalid upload file")
    else:
        raise RuntimeError("Invalid menu option received: upload file")

def recv_file_list():
    '''
    RECV_FILE_LIST:
        PARAMETER: None
        BRIEF: receives file list from the server and prints them to the user
        RETURN: None
    '''
    file_count = cli_socket.recv(4)
    file_count = int.from_bytes(file_count, "little", signed=True)
    received = 0
    data = b''

    while received < file_count:
        file_len = cli_socket.recv(8)
        file_len = int.from_bytes(file_len, "little", signed=True)
        data = cli_socket.recv(file_len)
        file_list.append(data.decode('utf-8'))
        received += 1
    print(file_list)

def get_file_list(menu_option):
    '''
    GET_FILE_LIST:
        PARAMETER: MENU_OPTION - menu option 3 for list file server directory
        BRIEF: sends command to the server to obtain list of current files
        RETURN: None
    '''
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
    '''
    MAIN:
        BRIEF: main method to call client application functionality
        USAGE: calls all necessary methods and disallows invalid input from the user
    '''
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
