# Problem-2: File transfer using Selective Repeat protocol over UDP

## Problem Statement

Write client and server programs to *upload* a given file (“input.txt”) from client to the server in a given scenario by implementing a reliable connection on top of **UDP** communication which uses **Selective Repeat** protocol. 

![Problem2 _ Scenario](./../Images/Problem2 _ Scenario.png?raw=true "Problem2 _ Scenario")

C uploads *input.txt* to S. All odd-numbered packets go through the relay node R1, while all evennumbered packets go through the relay node R2. R1 and R2 add a delay, which is a random number distributed uniformly between 0 to 2 ms for a given packet. *Acknowledgments can take any route and do not suffer from delays or packet drops*.

## Instructions to Run

- Open *four* terminals

- On first terminal **(server)**:

    1. `gcc server.c -o server`
    2. `./server`

- On second terminal **(relay 1)**:

    1. `gcc relay.c -lm -o relay`
    2. `./relay 1`

- On third terminal **(relay 2)**:

    1. `gcc relay.c -lm -o relay`
    2. `./relay 2`

- On fourth terminal **(client)**:

    1. `gcc client.c -o client`
    2. `./client {inputfile.txt}` (It takes the name of the input file as command line argument)  
    
- Output is stored in the file *output.txt*.

## Methodology

![Problem2 _ Working](./../Images/Problem2 _ Working.png?raw=true "Problem2 _ Working")

- Implemented a single timer per window basis

- Used select and fd_set to handle multiple connections.

- Managing file transfer over multiple connections: 

    - On the client side: 
        - Opened two sockets with different ports to establish two different UDP connections - one to relay 1 and the other to relay 2.
        - Whenever timeout occurrs, retransmit all unacknowledged packets in the window
        - If there was no timeout, check which ACK was received correctly and mark the proper acknowledgement in window.
        - Slide the window once all the packets in a window are acknowledged.

    - On the server side: 
        - Used simple UDP send and receive to receive *Data* from corresponding Relay 1/2 and direct *ACK* to the same relay.

- Relay
    - Kept relay number as #define since implementation of both relay is exactly same.
    - Randomly drops pocket to cause timeout
    - Causes a delay of randomly 0 to 2000 ms for each packet using system call `nanosleep()`
    - Opens one socket to client and one to server

- All the logs with *timestamp* are stored in the file *log_file.txt*