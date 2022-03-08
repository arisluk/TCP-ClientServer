# CS118 Project 2

## Makefile

This provides a couple make targets for things.
By default (all target), it makes the `server` and `client` executables.

It provides a `clean` target, and `tarball` target to create the submission file as well.

You will need to modify the `Makefile` to add your userid for the `.tar.gz` turn-in at the top of the file.

## Provided Files

`server.cpp` and `client.cpp` are the entry points for the server and client part of the project.

## Academic Integrity Note

You are encouraged to host your code in private repositories on [GitHub](https://github.com/), [GitLab](https://gitlab.com), or other places.  At the same time, you are PROHIBITED to make your code for the class project public during the class or any time after the class.  If you do so, you will be violating academic honestly policy that you have signed, as well as the student code of conduct and be subject to serious sanctions.

## Wireshark dissector

For debugging purposes, you can use the wireshark dissector from `tcp.lua`. The dissector requires
at least version 1.12.6 of Wireshark with LUA support enabled.

To enable the dissector for Wireshark session, use `-X` command line option, specifying the full
path to the `tcp.lua` script:

    wireshark -X lua_script:./confundo.lua

To dissect tcpdump-recorded file, you can use `-r <pcapfile>` option. For example:

    wireshark -X lua_script:./confundo.lua -r confundo.pcap

## TODO

Team:
Kevin Tang 805419480
Socket framework for client and server, congestion control.

Aris Luk 905326942
Packet formulation and transfer between client and server, error checks and timeouts.

Young-Ghee Hong 105213270
Server file writing, client file reads, error checks.

Design:
Client: The client first binds to a socket and establishes a connection with the server. Then it attempts a handshake. Then it repeatedly loops to send the entire requested file split into packets while also handling ACKs. Then it sends a FIN and waits for an ACK. Then it waits 2 seconds for stray FINs that it can ACK.
Server: The server first binds to a socket and listens for any opening connections. If it receives a SYN packet, it establishes a connection and saves all required data for that connection (id, seq number, time connected, etc). It then repeatedly loops to received all payload packets and writes it to the correct file. When it receives a FIN, it sends a FIN ACK and then closes the connection. It can handle multiple connections at once by sorting incoming packets according to connection id.

Problems: A main problem we had was figuring out how to have the client send multiple packets up to the cwnd without waiting for an ack. We solved this problem by using timeouts to keep sending packets while we didn't get any ACKs up to the cwnd.
Another problem we had was figuring out how to do timeouts and waits for packets, which we solved by using ctime and setsockopt.

Libraries used:
sys/select.h
sys/socket.h
sys/types.h
sys/utsname.h
sys/time.h
ctime
fstream
iostream
queue
string
thread
tuple
filesystem
dirent.h
vector

ACKs:
For socket timeouts:
https://stackoverflow.com/questions/39840877/c-recvfrom-timeout
For directory navigation:
https://c-for-dummies.com/blog/?p=3246