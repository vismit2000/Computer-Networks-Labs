## Problem-1: File transfer using multi-channel stop-and-wait protocol

### Problem Statement

Write client and server programs to upload a given file (“input.txt”) from client to the server using the given protocol by making **TCP** connections between the client and the server:

1. The sender transmits packets through two *different* channels (TCP connections).
2. The server acknowledges the receipt of a packet via the *same* channel through which the corresponding packet has been received.
3. The sender transmits a new packet using the *same* channel on which it has received an ACK for its one of the previously transmitted packet. Note that, at a time, there can be at most *two* outstanding unacknowledged packets at the sender side.
4. On the server-side, the packets transmitted through different channels may arrive *out of order*. In that case, the server has to buffer the packets temporarily to finally construct in-order data stream.

### Instructions to Run

- Open *two* terminals

- On first terminal **(server)**:

    1. `gcc server.c -o server`
    2. `./server`

- On second terminal **(client)**:

    1. `gcc client.c -o client`
    2. `./client {inputfile.txt}` (It takes the name of the input file as command line argument)
    
- Output is stored in the file *output.txt*.

### Methodology

- Used select and fd_set to handle multiple connections as required (using only one timer).

- Managing file transfer over multiple connections: 

    - On the client side: 
        - Opened two sockets with different ids and established two different connections.
        - Whenever timeout occurrs, *retransmit* the previous packet (there are at most two unacknowledged packets - one for Channel 0 and other for Channel 1)
        - If there was no timeout, check which ACK was received correctly. Send new packet on the channel that received the ACK correctly and retransmit the previous packet on the channel which did not receive ACK

    - On the server side: 
        - Handled multiple clients is by using `select()` command
        - Select command allows to monitor multiple file descriptors, waiting until one of the file descriptors become active.
        - For example, if there is some data to be read on one of the sockets select will provide that information.
        - Select works as an interrupt handler, which gets activated as soon as any file descriptor sends any data.

    - Data structure used for select: `fd_set`. It contains the list of file descriptors to monitor for some activity. There are four functions associated with fd_set:
        
        ```
        int FD_ZERO (fd_set *descriptorVector);                   /* removes all descriptors from vector */
        int FD_CLR (int descriptor, fd_set *descriptorVector);    /* remove descriptor from vector */
        int FD_SET (int descriptor, fd_set *descriptorVector);    /* add descriptor to vector */
        int FD_ISSET (int descriptor, fd_set *descriptorVector);  /* vector membership check */
        ```

    - Created a fd_set variable `readfds`, which will monitor all the active file descriptors of the clients plus that of the main server 
    listening socket.
    - Whenever a new client will `connect`, master_socket will be activated and a new fd will be open for that client. We will store its fd in our 
    client_list and in the next iteration we will add it to the `readfds` to monitor for *activity* from this client.
    - Similarly, if an old client sends some data, `readfds` will be activated and we will check from the list of existing client to see which 
    client has sent the data.