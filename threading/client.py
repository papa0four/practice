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


'''
SOCKET_INFO:
    PARAMETER: None
    BRIEF: parses cmdline arguments to get ip address and port info
    RETURN: server ip address and socket port
'''
def socket_info():
    # call argparse method to grab cmdline arguments
    parser = argparse.ArgumentParser()
    # creates a host argument to be caught
    parser.add_argument("host")
    # creates a port argument and ensures it is an int type value
    parser.add_argument("port", type=int)
    # parses the cmdline arguments
    args = parser.parse_args()
    # sets host variable to host cmdline argument
    host = args.host
    # sets port variable to port cmdline argument
    port = args.port
    # returns host and port variables
    return host, port

'''
SIGNAL_HANDLER:
    PARAMETER: SIG - signal to be caught
    PARAMETER: FRAME - stack frame pointer during the signal interruption
    BRIEF: catches the ctrl+c SIGINT signal from the user, 
            sends exit command to server
    RETURN: None
'''
def signal_handler(sig, frame):
    # set exit command
    command = 500
    # pack and send the initial exit command
    exit = struct.pack("I", command)
    sent = cli_socket.send(exit)
    # create exit message
    goodbye = "exit"
    # variable to send message in chunks
    totalsent = 0
    # iterate over message to ensure entire message is sent
    while totalsent < len(goodbye):
        sent = cli_socket.send(goodbye[totalsent:].encode('utf-8'))
        # check for send failures
        if sent == 0:
            raise RuntimeError("Socket connection broken: sending exit")
        # increment totalsent variable for loop
        totalsent += sent
    # print message, close socket and exit client application
    print("\nCtrl+C caught and handled... Goodbye!")
    cli_socket.close()
    sys.exit(0)

'''
GLOBAL: CLI_SOCKET, SERVER_ADDR, PORT
    BRIEF: creates the socket connection to the server
    USAGE: calls the socket_info method and creates the socket,
            exits program on failure
'''
# create client application socket
cli_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# call custom method to get socket info
server_addr, port = socket_info()
# begin try block
try:
    # connect to socket and print connection info
    cli_socket.connect((server_addr, port))
    print("Connected to {}:{}".format(server_addr, port))
# exception on socket connection failure
except OSError as msg:
    print("Closing socket on failure")
    # close socket and end client application
    cli_socket.close()
    cli_socket = None
    exit()

'''
MAIN_MENU:
    PARAMETER: None
    BRIEF: creates and displays a menu for the user to interact with
            only allows for int value input (1 - 5)
    RETURN: returns menu option selected on success, exits client program
            on invalid menu option
'''
def main_menu():
    # server menu display
    print("\n#--------------------------------------------------#")
    print("#-------------------FILE SERVER--------------------#")
    print("#--------------------------------------------------#")
    print("Please enter an option from the following menu:")
    print("1. Download Existing File from Server")
    print("2. Upload User File to Server")
    print("3. View Files Currently on the File Server")
    print("4. File Server Help")
    print("5. Exit File Server")
    # custom cmdline cursor prompt
    menu_option = input("/> ")
    # remove trailing carriage return
    menu_option.rstrip()
    # begin try block
    try:
        # cast menu selection to int type
        in_type = int(menu_option)
        # ensure value is between 1 and 5
        if in_type <= 5 or in_type >= 1:
            # return selection
            return in_type
    # value error exception
    except ValueError:
        print("Invalid menu option selected... Breaking connection")
        exit()
    

'''
HELP_OPTION:
    PARAMETER: MENU_OPTION - int value of 4
    BRIEF: prints a brief synopsis of how to use the client application
    RETURN: None
'''
# come back to this after setting up improved functionality
def help_option(menu_option):
    # ensure parameter is the help menu option
    if menu_option == 4:
        # display help transcript
        print("\n#--------------------------------------------------#")
        print("#--------------------HELP MENU---------------------#")
        print("#--------------------------------------------------#")
        print("Menu option 1 allows the user download files from the file server")
        print("Menu option 2 allows the user to upload files to the file server")
        print("Menu option 3 displays all downloadable files within the server")
        print("Menu option 5 allows the user to exit the file server program")
        print("If necessary, the user can enter 'ctrl+c' to gracefully exit the program")     

'''
EXIT_SERVER:
    PARAMETER: MENU_OPTION - int value of 5
    BRIEF: allows for a graceful client connection closure
    RETURN: None
'''
def exit_server(menu_option):
    # ensure parameter is exit menu option
    if menu_option == 5:
        # define exit command
        command = 500
        # pack the command value to be sent over socket and send command
        exit = struct.pack("I", command)
        sent = cli_socket.send(exit)
        # define exit message to be sent to server
        goodbye = "exit"
        # define variable to ensure all bytes are sent
        totalsent = 0
        # loop through and ensure entire message is sent
        while totalsent < len(goodbye):
            # send message to server
            sent = cli_socket.send(goodbye[totalsent:].encode('utf-8'))
            # check for bytes sent failure
            if sent == 0:
                raise RuntimeError("Socket connection broken: sending exit")
            # increment totalsent by actual bytes sent
            totalsent += sent
        # goodbye message and close socket
        print("Thank you.... Goodbye!")
        cli_socket.close()

'''
GET_FILE_SIZE:
    PARAMETER: None
    BRIEF: gets the size of the file to be used in the download_file method
    RETURN: FILE_SIZE - int value size of file to be downloaded
'''
def get_file_size():
    # receive file size bytes from server
    data = cli_socket.recv(8)
    # properly orders and translates the file size value
    file_size = int.from_bytes(data, byteorder='little', signed=True)
    # return the size of the file
    return file_size

'''
IS_CLIENT_FILE:
    PARAMETER: DIR - directory to check for valid client file
    PARAMETER: FILE - file to check for validity
    BRIEF: checks the validity of a directory and file passed by the user
            meant to be uploaded to the server
    RETURN: (bool) true on success (file/directory), false on failures
'''
def is_client_file(dir, file):
    # checks if dir is a valid file system location
    if os.path.isdir(dir):
        # creates the absolute path for the file to be uploaded
        path = dir + file
        # checks the validity of the file passed
        if os.path.isfile(path):
            print("OK")
            # return true on success false on failure
            return True
        else:
            return False
    # returns false on directory verification
    else:
        print("No such file or directory")
        return False

'''
DOWNLOAD_FILE:
    PARAMETER: None
    BRIEF: allows the client to download a specified file from the server
    RETURN: None, returns None on failure for execution
'''
def download_file():
    # call the get_file_size method
    requested_file_sz = get_file_size()
    # check for error
    if requested_file_sz == -1:
        print("Requested file not found on server...")
        # returns None on failure
        return None
    # prompt the user to enter a file name for storage
    newfile = input("Enter the name you wish to store your file as: ")
    # strip carriage return from input
    newfile = newfile.rstrip()
    # create absolute path for file storage
    download_path = "Client/" + newfile
    # set variable to allow for data transfer in chunks
    bytes_left = requested_file_sz
    print("The requested file is {} bytes in length".format(requested_file_sz))
    # begin try block
    try:
        # create and open file location in bytes/append mode
        with open(download_path, 'wb+') as file_ptr:
            # iterate over expected file size
            while bytes_left > 0:
                # recv file data in chunks of 1024
                contents = cli_socket.recv(1024)
                # check for download errors
                if contents is None:
                    raise RuntimeError("Socket connection broken: download")
                # write file bytes to newly created file
                file_ptr.write(contents)
                # decrement iterator for loop
                bytes_left -= 1024
        # display download completion to user
        print("Total bytes downloaded: {}".format(requested_file_sz))
        print("DONE")
    # except on file system error
    except OSError:
        print("Could not download file from server")


'''
DOWNLOAD_EXISTING_FILE:
    PARAMETER: MENU_OPTION - menu option 1
    BRIEF: sends download command to server and allows the user to obtain
            a file located on the server
    RETURN: None
'''
def download_existing_file(menu_option):
    # check if parameter is menu option 1
    if menu_option == 1:
        # set download command
        command = 200
        # pack the command value and send to server
        d_load = struct.pack("I", command)
        sent = cli_socket.send(d_load)
        # check for send error
        if sent == 0:
            raise RuntimeError("Socket connection broken: send download command")
        # prompt the user to enter the requested file
        choice = input("Type the name of the file you wish to download: ")
        # strip the carriage return from input
        choice = choice.rstrip()
        # totalsent variable for loop iteration
        totalsent = 0
        # ensure entire download message is sent to server
        while totalsent < len(choice):
            sent = cli_socket.send(choice[totalsent:].encode('utf-8'))
            # check for send failures
            if sent == 0:
                raise RuntimeError("Socket connection broken: choose action - file")
            totalsent += sent
        # print message to user and call download_file method
        print("You chose to download {} from the server".format(choice))
        download_file()
    # should never reach this statement but handles invalid parameter passed
    else:
        raise RuntimeError("Invalid menu option received: download existing")

'''
UPLOAD_FILE:
    PARAMETER: MENU_OPTION - menu option 2 for file upload
    BRIEF: allows the client to specify a file to send to the server
    RETURN: None
'''
def upload_file(menu_option):
    # check if parameter passed is upload menu option
    if menu_option == 2:
        # create upload command
        command = 300
        # pack and send upload command to server
        upload = struct.pack("I", command)
        sent = cli_socket.send(upload)
        # check for send error
        if sent == 0:
            raise RuntimeError("Socket connection broken: upload command")
        # create totalsent variable to send upload message in chunks
        totalsent = 0
        # create upload message
        send_file = "upload"
        # begin try block
        try:
            # create loop to ensure total message is sent to server
            while totalsent < len(send_file):
                sent = cli_socket.send(send_file[totalsent:].encode('utf-8'))
                # check for send error
                if sent == 0:
                    raise RuntimeError("Socket connection broken: send upload request")
                totalsent += sent
                # display upload progress to user
                print("Sent to upload request to server")
        # check for file system errors
        except OSError:
            raise RuntimeError("Could not send upload request command")
        # enter the directory location of files to upload
        user_in = input("Enter the directory location of your file: ")
        # strip trailing carriage return from input
        user_in = user_in.rstrip()
        # get current working directory
        cwd = os.getcwd()
        # set absolute path for file location
        dir = cwd + "/" + user_in + "/"
        # allow user to input the file to upload
        file = input("Enter the file you wish to upload: hint - include extension\n")
        #  strip trailing carriage return from input
        file = file.rstrip()
        # call is_client_file method to validate selected upload file
        if is_client_file(dir, file) is True:
            # create absolute path to file
            path = dir + file
            # get and send size of requested upload file
            file_size = os.path.getsize(path)
            size = struct.pack("I", file_size)
            sent = cli_socket.send(size)
            # check for send errors
            if sent == 0:
                raise RuntimeError("Socket connection broken: upload file size")
            # set and send length of file name
            name_len = len(file)
            n_len = struct.pack("I", name_len)
            sent = cli_socket.send(n_len)
            # check for send errors
            if sent == 0:
                raise RuntimeError("Socket connection broken: file name size")
            # variable to allow for file name to be sent in chunks
            totalsent = 0
            while totalsent < len(file):
                sent = cli_socket.send(file[totalsent:].encode('utf-8'))
                # check for send errors
                if sent == 0:
                    raise RuntimeError("Socket connection broken: file name")
                totalsent += sent
            # variable to allow for file data to be sent in chunks
            totalsent = 0
            print("Sending {} to File Server".format(path))
            # open requested upload file in read mode
            with open(path, "br") as file_ptr:
                # send file data in chunks of 1024 bytes
                while totalsent < file_size:
                    contents = file_ptr.read(1024)
                    sent = cli_socket.send(contents)
                    # check for send errors
                    if sent == 0:
                        raise RuntimeError("Socket connection broken: sending contents")
                    totalsent += sent
            # display upload completion message to user
            print("Upload Complete")
        # if requested file to upload is invalid or client terminates connection mid process
        else:
            # set error code and send to server
            err_code = 666
            size = struct.pack("I", err_code)
            sent = cli_socket.send(size)
            # check for send errors
            if sent == 0:
                raise RuntimeError("Socket connection broken: invalid upload file")
    # block should never be reached, raises invalid menu option error
    else:
        raise RuntimeError("Invalid menu option received: upload file")

'''
RECV_FILE_LIST:
    PARAMETER: None
    BRIEF: receives file list from the server and prints them to the user
    RETURN: None
'''
def recv_file_list():
    # receives the number of files currently on the file server
    file_count = cli_socket.recv(4)
    # converts the data received to appropriate int value
    file_count = int.from_bytes(file_count, "little", signed=True)
    # iteration variable to determine number of files received
    received = 0
    # declare empty bytes string
    data = b''
    # loop through and receive file list from server
    while received < file_count:
        # get the lengths of the file names and convert to int
        file_len = cli_socket.recv(8)
        file_len = int.from_bytes(file_len, "little", signed=True)
        # get actual file names and append to global file_list
        data = cli_socket.recv(file_len)
        file_list.append(data.decode('utf-8'))
        received += 1
    # display file list to user
    print(file_list)

'''
GET_FILE_LIST:
    PARAMETER: MENU_OPTION - menu option 3 for list file server directory
    BRIEF: sends command to the server to obtain list of current files
    RETURN: None
'''
def get_file_list(menu_option):
    # ensures parameter passed is menu option 3
    if menu_option == 3:
        # create list dir command and send to server
        request = 100
        list_dir = struct.pack("I", request)
        sent = cli_socket.send(list_dir)
        # check for send error
        if sent == 0:
            raise RuntimeError("Socket connection broken: get dir list")
        # displays list option selection to user
        print("Sent list directory command to server...")
        # variable for data loop
        totalsent = 0
        # list contents server message
        list_cmd = "ls"
        while totalsent < len(list_cmd):
            sent = cli_socket.send(list_cmd[totalsent:].encode('utf-8'))
            # check for send error
            if sent == 0:
                raise RuntimeError("Socket connection broken: sending list command")
            totalsent += sent
        # call recv_file_list method to display server contents
        recv_file_list()
    # should never reach this block, raises runtime error for invalid menu parameter
    else:
        raise RuntimeError("Invalid menu option received: get file")
    # clear global list variable
    file_list.clear()

'''
MAIN:
    BRIEF: main method to call client application functionality
    USAGE: calls all necessary methods and disallows invalid input from the user
'''
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
