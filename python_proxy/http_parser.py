import io
from enum import Enum, auto

class HttpParserState(Enum):
    START = auto()
    HEADERS = auto()
    BODY = auto()
    END = auto()

class HttpMessage(object):
    def __init__(self):
        self.parser_state = HttpParserState.START
        
        self.method = b''
        self.uri = b''
        self.version = b''
        self.headers: dict[bytes, bytes] = {}

        self.body = b''

        # To store last packet's data if fragmentation happens
        self.residual = b''
        

    def parse(self, msg: bytes):
        bs = io.BytesIO(self.residual + msg)
        # First line has to be the request line, but it's not guranteed that msg is at the start of the request due to fragmentation
        # NOTE: http do ask people to send the header in full so unlikely to have fragmentation over here tbh
        if self.parser_state == HttpParserState.START:
            request = bs.readline()

            if not request.endswith(b'\n'):
                self.residual =  request
                return 
            self.residual == b''
            
            self.method, self.uri, self.version = request.rstrip().split(b' ')
            self.parser_state = HttpParserState.HEADERS


        if self.parser_state == HttpParserState.HEADERS:
            while True:
                header_new_line = bs.readline()
                
                # NOTE: Packet Fragment could happen exactly at the end of line, lead to empty readline not CLRF
                # Or there's just no header at all
                if header_new_line == b'':
                    return

                if not header_new_line.endswith(b'\n'):
                    self.residual = header_new_line
                    return
                self.residual == b''

                # End of the header is just a CLRF, but to be resilient we should also check \n without \r
                if header_new_line == b'\r\n' or header_new_line == b'\n':
                    # No need to read body for GET
                    if self.method == b'GET':
                        self.parser_state = HttpParserState.END
                    else:
                        self.parser_state = HttpParserState.BODY

                    break


                name, value = header_new_line.split(b':', maxsplit=1)
                value = value.strip()
                self.headers[name] = value

            #print(f"Finish read headers: {self.headers}")

        if self.parser_state == HttpParserState.BODY:
            self.body += bs.read()
    

    def get_message_bytes(self) -> bytes:
        '''
        Returns the full http message as bytes that can be send through sockets
        '''
        b_clrf: bytes = b'\r\n'
        b_request_separator: bytes = b' '
        b_header_field_separator: bytes = b':'

        # Reconstruct request line
        res: bytes = self.method + (b_request_separator) + self.uri + (b_request_separator) + self.version + b_clrf

        # Then headers
        for name, val in self.headers.items():
            res = res + name + b_header_field_separator + val + b_clrf
        res += b_clrf

        # Finally, the body
        res += self.body

        return res


    def should_keep_alive(self) -> bool:
        connection = self.headers.get('Connection')

        if self.version == b'HTTP/1.0':
            return connection and connection.lower() == b'keep-alive'
        
        if self.version == b'HTTP/1.1':
            return not (connection and connection.lower() == b'close')

# test = b'asdf\r\n'
# print(test.endswith(b'\r\n'))