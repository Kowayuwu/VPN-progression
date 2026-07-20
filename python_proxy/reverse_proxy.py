'''
Get http request, forward to upstream server, return back the response to client

Assumes the http server runs at port 9000 (make sure a website is running)

-
Keeps the connection alive for HTTP 1.0 1.1
For HTTP 1.0: Close connection after each request unless specified connection keep-alive

For HTTP 1.1: Make connection keep-alive as default, only close if specified in the connection field (or missing connection field)
-

-
Concurrently handles client using io multiplexing
-

'''
import socket
from http_parser import HttpMessage, HttpParserState
import select

BASIC_SERVER_PORT: int = 9000
LISTENING_PORT: int = 8000

BUFFER: int = 1024

TARGET_ADDR: tuple = ("localhost", BASIC_SERVER_PORT)

io_input: list[socket.socket] = []
io_output: list[socket.socket] = []
io_to_send: dict[socket.socket, tuple[HttpMessage, bool]] = {}
client_request_dict: dict[socket.socket, HttpMessage] = {}


def clean_client_sock(client_socket: socket.socket):
    client_socket.close()
    print('Connection to client closed')

    client_request_dict.pop(client_socket, None)
    io_input.remove(client_socket)
    io_output.remove(client_socket)


def deal_with_client(client_socket: socket.socket) -> None:
    '''
    Called ONLY when client is readable, call recv once to retrieve data from client
    Action:
        1. recv from client, create entry in client_request_dict
        2. send request to server if request is completed and store response in io_to_send
        3. check if keep_alive is needed
    
    cleans the client socket if necessary, e.g. connection closed
    '''

    client_request = client_request_dict.get(client_socket, HttpMessage())
    client_request_dict[client_socket] = client_request
    
    msg_in: bytes = client_socket.recv(BUFFER)
    print(f"<-    received {len(msg_in)} bytes of data from client ---           ")
    client_request.parse(msg_in)

    # Client sent FIN, delete entry and end process
    if not msg_in:
        clean_client_sock(client_socket)
        return

    print(f"\nHTTP VERSION {client_request.method} with HEADERS {client_request.headers}\n")

    # if request is fragmented don't send to upstream yet!
    if client_request.parser_state != HttpParserState.END:
        print("request fragmented")
        return


    # Client request construction finished, connect to server TODO: make persistant connection
    upstream_socket = socket.socket(family=socket.AF_INET, type=socket.SocketKind.SOCK_STREAM)
    upstream_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    upstream_socket.connect(("localhost", BASIC_SERVER_PORT))


    # Send msg to upstream, delete the request afterwards
    try:
        upstream_socket.sendall(client_request.get_message_bytes())
        print(f"->    -- sent request to upstream with {len(client_request.get_message_bytes())} amount of bytes --   ")

    except Exception as e:
        print(f"error when sending request to upstream {e}")
        upstream_socket.close()
        client_socket.send(b'HTTP/1.1 500 error\r\n\r\n')
        return
    
    finally:
        client_request_dict.pop(client_socket, None)


    # Get repsonse back from server and add to to_send waitlist
    server_response = HttpMessage()
    while True:
        chunk = upstream_socket.recv(BUFFER)
        if not chunk:
            break
        server_response.parse(chunk)
        print(f"<-    ---           received {len(chunk)} bytes of data from upstream")
    io_to_send[client_socket] = (server_response, client_request.should_keep_alive())

    # Close the connection to upstream after we get the response
    # TODO: make upstream connection persistant as well
    upstream_socket.close()

    



def send_response(client_socket: socket.socket):
    '''
    Send server response back to the given client socket, and remove the response from io_to_send
    If client request does not want keep alive, close and clean the client socket
    '''
    server_response, should_keep_alive = io_to_send.get(client_socket, (None, False))

    if server_response:
        client_socket.sendall(server_response.get_message_bytes())
        print(f"->      -- sent response back to client --   ")
    else:
        print(f"server response is faulty")

    io_to_send.pop(client_socket, None)

    # TODO: test this
    if not should_keep_alive:
        print("Client does not want to keep socket alive")
        clean_client_sock(client_socket)


# Start
if __name__ == '__main__':
    # connect to client
    bind_socket = socket.socket(family=socket.AF_INET, type=socket.SocketKind.SOCK_STREAM)
    bind_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    bind_socket.setblocking(False)
    bind_socket.bind(("localhost", LISTENING_PORT))
    bind_socket.listen(5)
    print(f"listening on port {LISTENING_PORT}, accepting new connection")
    
    io_input.append(bind_socket)

    while True:
        readable, writeable, exceptional = select.select(io_input, io_output, io_input)

        for fd in readable:

            # accept new clients and add them to io multiplexing watch list
            if fd == bind_socket:
                client_socket, client_addr = bind_socket.accept()
                client_socket.setblocking(False)
                io_input.append(client_socket)
                io_output.append(client_socket)
                print(f"connected to client with addr {client_addr}, currently we have {len(io_input)-1} clients")
            
            # currently, all other fd that are NOT the bind socket are client sockets
            else:
                deal_with_client(client_socket=client_socket)

        for fd in writeable:
            if fd in io_to_send:
                send_response(fd)

        for fd in exceptional:
            print(f"exceptional: {fd}")