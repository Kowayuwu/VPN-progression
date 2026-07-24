import io
from enum import Enum, auto

class HttpParserState(Enum):
    START = auto()
    HEADERS = auto()
    BODY = auto()
    END = auto()

class HTTPMessageType(Enum):
    REQUEST = auto()
    RESPONSE = auto()

class HttpMessage(object):
    def __init__(self, type: HTTPMessageType):
        self.parser_state = HttpParserState.START
        self.message_type = type
        self.start_line: bytes = b''
        
        # For requests
        self.method = b''
        self.uri = b''
        self.version = b''

        # For responses:
        self.version = b''
        self.status_code = b''
        self.status_message = b''

        self.headers: dict[bytes, bytes] = {}
        self.body = b''

        # To store last packet's data if fragmentation happened
        self.residual: bytes = b''
        

    def parse(self, msg: bytes):
        '''
        Given the HTTP Message bytes, parse them while updating the state
        Inform whether we finished parsing by updating the state to HttpParserState.END
        '''
        bs = io.BytesIO(self.residual + msg)
        # It's not guranteed that msg is at the start when parse is called due to fragmentation
        # NOTE: http do ask people to send the header in full so unlikely to have fragmentation over here tbh
        if self.parser_state == HttpParserState.START:
            start_line = bs.readline()
            self.start_line = start_line

            if not start_line.endswith(b'\n'):
                self.residual =  start_line
                return 
            self.residual == b''
            
            if self.message_type == HTTPMessageType.REQUEST:
                self.method, self.uri, self.version = start_line.rstrip().split(b' ')
                self.parser_state = HttpParserState.HEADERS
            else:
                # for responses we max split = 2 because status message can contain spaces
                try:
                    self.version, self.status_code, self.status_message = start_line.rstrip().split(b' ', 2)
                    self.parser_state = HttpParserState.HEADERS
                except:
                    # Found out that if the request is like really really bad the response can start with <!DOCTYPE HTML>
                    self.parser_state = HttpParserState.BODY


        if self.parser_state == HttpParserState.HEADERS:
            while True:
                header_new_line = bs.readline()
                # print(f"header new line {header_new_line}")
                
                # NOTE: Packet Fragment could happen exactly at the end of line, leading to empty readline not CLRF
                # Or, if there's just no header at all
                if header_new_line == b'':
                    return

                if not header_new_line.endswith(b'\n'):
                    self.residual = header_new_line
                    return
                self.residual == b''

                # End of the header is just a CLRF, but to be resilient we should also check \n without \r
                if header_new_line == b'\r\n' or header_new_line == b'\n':
                    # No need to read body for GET
                    if self.message_type == HTTPMessageType.REQUEST and self.method == b'GET':
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
    

    def add_header(self, key: str, value: str):
        '''
        Just adding whatever header you want, not doing any checks
        '''
        self.headers[key.encode()] = value.encode()



    def to_bytes(self) -> bytes:
        '''
        Returns the full http message as bytes that can be send through sockets
        '''
        b_clrf: bytes = b'\r\n'
        b_startline_separator: bytes = b' '
        b_header_field_separator: bytes = b':'

        # Reconstruct starting line
        # if self.message_type == HTTPMessageType.REQUEST:
        #     res: bytes = b_startline_separator.join([self.method, self.uri, self.version]) + b_clrf
        # else:
        #     res: bytes = b_startline_separator.join([self.version, self.status_code, self.status_message]) + b_clrf
        res = self.start_line

        # Then headers
        for name, val in self.headers.items():
            res = res + name + b_header_field_separator + val + b_clrf
        res += b_clrf

        # Finally, the body
        res += self.body

        return res


    def should_keep_alive(self) -> bool:
        '''
        Returns whether the socket connection wants to keep alive by checking HTTP Message information against HTTP rules

        For ex, HTTP/1.1 defaults to keep alive while HTTP/1.0 defaults to NOT keep alive unless stating it in the 'Connection' header field
        '''
        connection = self.headers.get('Connection')

        if self.version == b'HTTP/1.0':
            return connection and connection.lower() == b'keep-alive'
        
        if self.version == b'HTTP/1.1':
            return not (connection and connection.lower() == b'close')

# test = b'asdf\r\n'
# print(test.endswith(b'\r\n'))