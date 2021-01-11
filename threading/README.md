# Synopsis
The code within this repo is meant to be an example File Transfer Server/Client application. It allows the user to trasnfer files *in binary format* between a client application, written in python and a server application written in C. This is a PROOF OF CONCEPT and is a multi-threaded application, allowing multiple clietns to connect to the server. The user will be able to specify the amount of clients allowed to connect to the server by running the program and providing a number after the *-t* flag within the command line. As of now, the most allowable clients is 50 but can be changed if need be.

For proof of concept, the server application assumes that a *FileServer/* directory exists within the working path where the executable code is located. This is where the files within the server are located and can be accessed by the application. Additionally, the client application assumes that all client files are located within the *Client/* directory, again within the same folder where the application code resides. It is assumed that this is where the client files are located.

## If you do not have a FileServer/ and Client/ directory
You can simply make them and move the relevant practice files within. That way the application understands where to look for the appropriate files

## If hardcoded locations is not desirable for PROOF OF CONCEPT
I, the writer of the application, can change the code to allow both applications to request directory locations for file storage, rather than having hard-coded locations in place.

# Server Usage
To run the File Transfer Program, you can use the Makefile, which compiles the server application with the following flags: *-g -Wall -Werror -Wextra -Wpedantic -std=c11 -pthread*

If not creating a personal *run* script, you can launch the application by typing the following into the command line:
*./serv.out -p [port number to run the server on] -t [specified number of clients]*

## If creating a *run* script
It is advised to use the following within an executable file: <br />
--- EXAMPLE --- <br />
make clean && \ <br />
make && \ <br />
./serv.out -p 31337 -t 10

---> chmod +x run && ./run

# Client Usage
Because the client application is written in python, the user can enter the following into the command line:
python3 client.py 127.0.0.1 [ip address of the server] 31337 [specified port used]

Once run, the client application menu will appear to the user. Additionally, built into the application [menu option 4] is a brief help menu to explain what each option allows. While running, the user can select an option from the menu [1 - 5] where each will briefly prompt the user as to the next step, **SO PLEASE READ EACH PROMPT**