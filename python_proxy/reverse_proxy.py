# get http request, forward to upstream server, return back the response to client
import socket

BASIC_SERVER_PORT = 9000
LISTENING_PORT = 8000

BUFFER = 1024

TARGET_ADDR = ("localhost", BASIC_SERVER_PORT)

# connect to client
bind_socket = socket.socket(family=socket.AF_INET, type=socket.SocketKind.SOCK_STREAM)
bind_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
bind_socket.bind(("localhost", LISTENING_PORT))
bind_socket.listen(5)



while True:
    # get request from client
    print(f"listening on port {LISTENING_PORT}, accepting new connection")
    client_socket, client_addr = bind_socket.accept()
    print(f"connected to client with addr {client_addr}")
    msg_in = client_socket.recv(1024)
    print(f"received {len(msg_in)} bytes of data")


    # connect to server
    upstream_socket = socket.socket(family=socket.AF_INET, type=socket.SocketKind.SOCK_STREAM)
    upstream_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    upstream_socket.connect(("localhost", BASIC_SERVER_PORT))


    # send msg to upstream
    try:
        upstream_socket.sendall(msg_in)
    except Exception as e:
        print(f"error when sending msg to upstream {e}")
        upstream_socket.close()
        client_socket.send(b'HTTP/1.1 500 error\r\n\r\n')
        continue



    # send repsonse back to client
    response = b''
    while True:
        chunk = upstream_socket.recv(BUFFER)
        if not chunk:
            break
        response += chunk
    print(f"received {len(response)} amount of bytes back from the server, sending it to the client")


    client_socket.sendall(response)

    # TODO: make connection longing
    client_socket.close()
    upstream_socket.close()
