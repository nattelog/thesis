"""
A python version of the gateway. Used primarily for bootstraping the rest of
the model.
"""

import threading
from log import now, Log, StandardWriter
from net import Stub, TCPServer

def nth_prime(n):
    def is_prime(v):
        return all(v % k > 0 for k in range(2, v))

    i = 0
    j = 1

    while i < n+1:
        if is_prime(j):
            i += 1
        j += 1

    return j - 1

def do_cpu(intensity):
    """
    Simulates a CPU intensive task by calculating the nth prime, where n is
    2^10 * intensity, and intensity varies between 0 and 1.
    """

    if intensity < 0 or intensity > 1:
        raise AttributeError(
            'Cannot simulate CPU intensity: value must be between 0 and 1')

    n = int((2 ** 12) * intensity)
    nth_prime(n)

class GatewayAPI():
    """ API callable by the nameservice.
    """

    def __init__(self, start_test_event):
        self.start_test_event = start_test_event

    def get_timestamp(self):
        return now()

    def start_test(self):
        self.start_test_event.set()

class GatewayServer(threading.Thread):

    logger = Log.get_logger('GatewayServer')

    def __init__(self):
        threading.Thread.__init__(self)
        self.daemon = True
        self.test_start_event = threading.Event()
        self.server = TCPServer(('', 0), GatewayAPI(self.test_start_event))

    def run(self):
        while True:
            self.server.accept()

    def hostname(self):
        return self.server.hostname()

    def wait_for_start(self):
        self.test_start_event.wait()

    def close(self):
        self.server.close()

class PassiveGateway():
    """ A passive gateway is a TCP client that requests events from passive
    devices.
    """

    logger = Log.get_logger('PassiveGateway')
    local_logger = Log.get_logger('PassiveGatewayLocal', StandardWriter)

    def __init__(self, nsaddress, cpu_intensity, io_intensity):
        self.server = GatewayServer()
        self.ns = Stub(nsaddress)
        self.cpu_intensity = cpu_intensity
        self.io_intensity = io_intensity

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

    def wait_for_start(self):
        self.server.wait_for_start()

    def get_events(self):
        for did in self.devices:
            if (self.ns.status(did) == 1):
                event = self.ns.next_event(did)
                PassiveGateway.logger.info('EVENT_LIFECYCLE_RETRIEVED:{}',
                        event)
                PassiveGateway.logger.info('EVENT_LIFECYCLE_DISPATCHED:{}',
                        event)
                do_cpu(self.cpu_intensity)
                PassiveGateway.logger.info('EVENT_LIFECYCLE_DONE:{}',
                        event)
