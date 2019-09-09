# remote-file-transfer-c
A project written in C, this is a system used for transferring files between two computer systems, a client and a server remotely via sockets using the sys/socket.h UNIX library and synchronous I/O multiplexing.

# Usage
Compile [server.c](server.c) and [client.c](client.c) separately on two different UNIX machines connected to the internet. Run the server initally on one machine and then run the client and connect to the server afterward. The file requested from the server machine can be specified in the arguments of the client code. The requested file is then delivered to the client machine if available.</br>

Assuming [server.c](server.c) has been compiled to 'server' and [client.c](client.c) has been compiled to 'client' on the current directory,</br>
To run 'server':
```c
./server port_no
```
where `port_no` is the port number to register the server on.</br>
To run 'client':
```c
./client ip_address port_no filename_to_get filename_to_save
```
where `ip_address` is the IPv4 address of the machine running the server code, `port_no` is the port number that the server code has been registered on, `filename_to_get` is the name of the file to get on the server machine, `filename_to_save` is the name of the file to save as on the client machine.
