"""
A python version of the gateway. Used primarily for bootstraping the rest of
the model.
"""

import socket
import json
import time
import sys
import getopt
from log import Log, UDPWriter, StandardWriter
from testmanager import LOG_SERVER_PORT, NAMESERVICE_PORT


class Stub():
    """ Wraps the socket request to the device.
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


class PassiveGateway():
    """ A passive gateway is a TCP client that requests events from passive
    devices.
    """

    logger = Log.get_logger('PassiveGateway')
    local_logger = Log.get_logger('PassiveGatewayLocal', StandardWriter)

    def __init__(self, nsaddress, argv):
        ns = Stub(nsaddress)
        ns.verify_gateway()
        self.devices = ns.hostnames()
        PassiveGateway.local_logger.info(
            'Retrieved devices from nameserver on {}: {}',
            nsaddress, self.devices)

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
