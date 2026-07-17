'''
Get http request, forward to upstream server, return back the response to client

Assumes the http server runs at port 9000 (make sure a website is running)

Keeps the connection alive for HTTP 1.0 1.1
For HTTP 1.0: Close connection after each request unless specified connection keep-alive

For HTTP 1.1: Make connection keep-alive as default, only close if specified in the connection field (or missing connection field)


'''
import socket
from http_parser import HttpMessage, HttpParserState

BASIC_SERVER_PORT = 9000
LISTENING_PORT = 8000

BUFFER = 1024

TARGET_ADDR = ("localhost", BASIC_SERVER_PORT)

def deal_with_client(client_socket: socket):
    while True: # For each http request

        client_request = HttpMessage()

        while client_request.parser_state != HttpParserState.END:
            msg_in: bytes = client_socket.recv(BUFFER)
            print(f"<-    received {len(msg_in)} bytes of data from client ---           ")
            client_request.parse(msg_in)

            # Client sent FIN, end dealing with this socket
            if not msg_in:
                # TODO: idk i feel like something needs to be here after implementing upstream consistent connection
                return

        print(f"\nHTTP VERSION {client_request.method} with HEADERS {client_request.headers}\n")


        # Connect to server TODO: make persistant connection
        upstream_socket = socket.socket(family=socket.AF_INET, type=socket.SocketKind.SOCK_STREAM)
        upstream_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        upstream_socket.connect(("localhost", BASIC_SERVER_PORT))


        # Send msg to upstream
        try:
            upstream_socket.sendall(client_request.get_message_bytes())
            print(f"->    -- sent request to upstream with {len(client_request.get_message_bytes())} amount of bytes --   ")

        except Exception as e:
            print(f"error when sending request to upstream {e}")
            upstream_socket.close()
            client_socket.send(b'HTTP/1.1 500 error\r\n\r\n')
            continue


        # Get repsonse back from server
        server_response = HttpMessage()
        while True:
            chunk = upstream_socket.recv(BUFFER)
            if not chunk:
                break
            server_response.parse(chunk)
            print(f"<-    ---           received {len(chunk)} bytes of data from upstream")

        
        # print(f"server headers: {server_response.headers}")
        client_socket.sendall(server_response.get_message_bytes())
        print(f"->      -- sent response back to client --   ")


        # TODO: make upstream connection persistant as well
        upstream_socket.close()

        if not client_request.should_keep_alive():
            print("End deal with client, not keeping socket alive")
            return

        

if __name__ == '__main__':
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

        deal_with_client(client_socket=client_socket)


        client_socket.close()
        print('Connection to client closed')
