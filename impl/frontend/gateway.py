"""
A python version of the gateway. Used primarily for bootstraping the rest of
the model.
"""


import time
import threading
from log import now, Log, StandardWriter
from net import Stub, TCPServer


class GatewayAPI():
    """ API callable by the nameservice.
    """

    def get_timestamp(self):
        return now()


class GatewayServer(threading.Thread):

    logger = Log.get_logger('GatewayServer')

    def __init__(self):
        threading.Thread.__init__(self)
        self.daemon = True
        self.server = TCPServer(('', 0), GatewayAPI())

    def run(self):
        while True:
            self.server.accept()

    def hostname(self):
        return self.server.hostname()

    def close(self):
        self.server.close()


class PassiveGateway():
    """ A passive gateway is a TCP client that requests events from passive
    devices.
    """

    logger = Log.get_logger('PassiveGateway')
    local_logger = Log.get_logger('PassiveGatewayLocal', StandardWriter)

    def __init__(self, nsaddress, argv):
        self.server = GatewayServer()
        self.ns = Stub(nsaddress)

    def start_server(self):
        self.server.start()

    def close_server(self):
        self.server.close()

    def verify_configuration(self, configuration):
        addr = self.server.hostname()
        return self.ns.verify_gateway(configuration, addr)

    def register_devices(self):
        self.devices = self.ns.hostnames()
        PassiveGateway.local_logger.info(
            'Retrieved devices from nameserver: {}',
            self.devices)

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
