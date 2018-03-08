import threading
import socket
import time
import json
from log import Log, StandardWriter

class Stub():
    """ Wraps the socket request to support RMI.
    """

    logger = Log.get_logger('Stub', StandardWriter)

    def __init__(self, address):
        self.address = tuple(address)

    def _rmi(self, method, *args):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(self.address)
        conn = sock.makefile(mode='rw')

        payload = json.dumps({
            'method': method,
            'args': args
        })

        Stub.logger.debug('{}: <<<< {}', sock.getsockname(), payload)

        conn.write(payload + '\n')
        conn.flush()

        response = json.loads(conn.readline())
        Stub.logger.debug('{}: >>>> {}', sock.getsockname(), response)

        if 'result' in response:
            return response['result']

        if 'error' in response:
            err = response['error']
            name = str(err['name'])
            err_args = err['args']
            E = type(name, (Exception, ), {})

            raise E(*err_args)

    def __getattr__(self, attr):
        def rmi_call(*args):
            return self._rmi(attr, *args)
        return rmi_call

class Request(threading.Thread):
    """ Representation of a request coming from the network.
    """

    logger = Log.get_logger('Request')

    def __init__(self, api, conn, addr, delay=0):
        threading.Thread.__init__(self)
        self.api = api
        self.conn = conn
        self.addr = addr
        self.delay = delay
        self.daemon = True

    def process_request(self, request):
        try:
            Request.logger.debug('{}:>>>> {}', self.addr, repr(request))
            data = json.loads(request)
            method = data['method']
            args = data['args']
            result = getattr(self.api, method)(*args)
            result = json.dumps({ 'result': result })
            Request.logger.debug('{}:<<<< {}', self.addr, repr(result))

            return result
        except Exception as e:
            result = json.dumps({
                'error': { 'name': type(e).__name__, 'args': [e.message] }
            })
            Request.logger.error('{}:<<<< {}', self.addr, repr(result))
            return result

    def run(self):
        try:
            worker = self.conn.makefile(mode='rw')
            request = worker.readline()

            if self.delay > 0:
                # wait both before and after the processing
                Request.logger.debug('{}:Delay {} s', self.addr, self.delay)
                time.sleep(self.delay)

            result = self.process_request(request)

            if self.delay > 0:
                time.sleep(self.delay)

            worker.write(result + '\n')
            worker.flush()
        except Exception as e:
            Request.logger.error('The gateway connection has died: {}: {}',
                    type(e), e);
        finally:
            Request.logger.debug('{}: Close', self.addr)
            self.conn.close()

class TCPServer():
    """ Handles incoming TCP connections and requests on the socket. All
    requests are handled in a new Request thread.
    """

    logger = Log.get_logger('TCPServer')

    def __init__(self, address, api, delay=0):
        self.address = address
        self.api = api
        self.delay = delay
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.bind(self.address)
        self.server.listen(5)
        TCPServer.logger.debug('{}: Start', self.hostname())

    def close(self):
        hostname = self.hostname()
        self.server.close()
        TCPServer.logger.debug('{}: Close', hostname)

    def accept(self):
        try:
            conn, addr = self.server.accept()
            TCPServer.logger.debug('{}: Accept', addr)
            req = Request(self.api, conn, addr, self.delay)
            req.start()
        except socket.error:
            TCPServer.logger.error(socket.error)

    def hostname(self):
        return self.server.getsockname()
