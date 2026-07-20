'''
Just trying to make a simple io multiplexing example, then integrate it to the main project later

using selet() cuz it works on most machines

Use epoll() for linux, or kqueue() for Mac / BSDs for better performance 

You shouldn't forget this but fd = file descriptor
'''

import socket
import select
PORT: int = 18888
BUFFER = 1024

SIMPLE_MSG_BACK = b'HTTP/1.0 200 ok\r\n\r\n yo hi'

bind_socket = socket.socket(family=socket.AF_INET, type=socket.SOCK_STREAM)
bind_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# We need to iterate through different fd to check them concurrently
# by default, functions like accept() blocks which then don't allow the following fds to get checked
# so we need to SET THE SOCKET TO NON-BLOCKING
bind_socket.setblocking(False)

bind_socket.bind(("localhost", PORT))
bind_socket.listen(10)

# Add all fds here, inputs are for reading (e.g. recv)
inputs = [bind_socket]

# Outputs are for writing (e.g. send)
outputs = []
# Not all writable fd should be sending out message though, so make another list for fds that need to send msg
# think - we only want to send back response when we get a request. BUT all connected sockets are always writable even without getting a request
# using a set because we don't want to send the same msg twice
to_send = set()
while True:
    readable, writeable, exceptional = select.select(inputs, outputs, inputs)

    for fd in readable:
        if fd == bind_socket:
            client_sock, client_addr = fd.accept()

            # CRUCIAL! set non-blocking so actions like recv don't block as well
            client_sock.setblocking(False)
            inputs.append(client_sock)
            outputs.append(client_sock)

            print(f"connected to client addr {client_addr}, currently we have {len(inputs)-1} amount of clients")
        
        # others should be all client sockets
        else:
            msg = fd.recv(BUFFER)

            if msg:
                print(f"recv from client: {msg}")
                to_send.add(fd)
            else:
                # FIN was sent, handle that
                fd.close()
                inputs.remove(fd)
                outputs.remove(fd)
                to_send.discard(fd) # NOTE: fd MIGHT not be in to_send

    for fd in writeable:
        if fd in to_send:
            fd.send(SIMPLE_MSG_BACK)
            to_send.remove(fd)

    # Don't care in this scope
    for fd in exceptional:
        print(f"Exception fd: {fd}")