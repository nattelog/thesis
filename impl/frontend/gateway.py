"""
A python version of the gateway. Used primarily for bootstraping the rest of
the model.
"""

import socket
import json
import time
from log import Log, UDPWriter
from testmanager import LOG_SERVER_ADDR, NAMESERVICE_ADDR


class Stub():
    """ Wraps the socket request to the device.
    """

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

        conn.write(payload + '\n')
        conn.flush()

        response = json.loads(conn.readline())

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


class PassiveGateway():
    """ A passive gateway is a TCP client that requests events from passive
    devices.
    """

    logger = Log.get_logger('PassiveGateway')

    def __init__(self, nsaddress):
        ns = Stub(nsaddress)
        self.devices = ns.hostnames()

    def get_events(self):
        for daddress in self.devices:
            device = Stub(daddress)
            if (device.status() == 1):
                event = device.next_event()
                PassiveGateway.logger.info('EVENT_LIFECYCLE_RETRIEVED:{}',
                        event)
                time.sleep(1)
                PassiveGateway.logger.info('EVENT_LIFECYCLE_DISPATCHED:{}',
                        event)
                time.sleep(1)
                PassiveGateway.logger.info('EVENT_LIFECYCLE_DONE:{}',
                        event)


if __name__ == '__main__':
    Log.config(
            level=Log.LEVEL_DEBUG,
            default_writer=UDPWriter(LOG_SERVER_ADDR))
    gw = PassiveGateway(NAMESERVICE_ADDR)

    print('output piped to log server on {}'.format(LOG_SERVER_ADDR))

    while True:
        gw.get_events()
        time.sleep(0.5)
