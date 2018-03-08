"""
Client model. Highly inspired by the lab series in the course "Distributed
Systems", TDDD25 at Linkoping University. Implements a remote method invocation
model.
"""

import threading
import Queue
import socket
import uuid
from log import Log
from net import TCPServer, Stub

class EventError(Exception):
    def __init__(self, message):
        self.message = message

class Event:
    """ Event class containing information for an event.
    """

    def __init__(self):
        self.id = uuid.uuid4()

    def __str__(self):
        return str(self.id)

class Device:
    """ The local device abstraction.
    """

    logger = Log.get_logger('Device')

    def __init__(self):
        self.event_queue = Queue.Queue(10);

    def status(self):
        """ 0 if no event is ready to be read, 1 otherwise.
        """

        return 0 if self.event_queue.empty() else 1

    def put_event(self, event):
        """ Put a new event in the queue. Does nothing if full.
        """

        try:
            self.event_queue.put_nowait(event)
            Device.logger.info('EVENT_LIFECYCLE_CREATED:{}', event)
        except Queue.Full:
            return

    def next_event(self):
        """ Returns the next event in the queue. Throws if queue is empty.
        """

        if self.event_queue.empty():
            raise EventError('Cannot get next event: queue is empty.')

        event = self.event_queue.get_nowait()
        Device.logger.info('EVENT_LIFECYCLE_FETCHED:{}', event)

        return str(event)

class Producer(threading.Thread):
    """ Produces new events and puts them on the device event queue.
    """

    logger = Log.get_logger('Producer')

    def __init__(self, device, frequency):
        threading.Thread.__init__(self)
        self.daemon = True
        self.device = device
        self.frequency = frequency
        self.stop_event = threading.Event()

    def run(self):
        Producer.logger.debug('{}: Start', id(self))

        if self.frequency <= 0:
            return

        while not self.stop_event.wait(1 / self.frequency):
            event = Event()
            self.device.put_event(event)
            Producer.logger.debug('{}: Put event \'{}\'', id(self), event)

    def stop(self):
        self.stop_event.set()
        Producer.logger.debug('{}: Stop', id(self))

class PassiveDevice(threading.Thread):
    """ A passive device is a socket server listening for requests that call
    methods on its event queue, and returns the result.
    """

    logger = Log.get_logger('PassiveDevice')

    def __init__(self, address, frequency, delay):
        threading.Thread.__init__(self)
        self.daemon = True
        self.device = Device()
        self.server = TCPServer(address, self.device, delay)
        self.producer = Producer(self.device, frequency)

    def run(self):
        self.producer.start()
        PassiveDevice.logger.info('{}: Start', self.hostname())

        while True:
            self.server.accept()

    def stop(self):
        hostname = self.hostname()
        self.producer.stop()
        self.server.close()
        PassiveDevice.logger.info('{}: Stop', hostname)

    def hostname(self):
        return self.server.hostname()

class NameServiceAPI:
    """ API callable by the gateway.
    """

    def __init__(self, devices, configuration, gateway_event):
        self.devices = devices
        self.gateway_event = gateway_event
        self.configuration = configuration

    def hostnames(self):
        return [device.hostname() for device in self.devices]

    def verify_gateway(self, gw_configuration, address):
        """ Called over the network by the gateway to verify it has been called
        with the correct configuration. Updates the gateway address in the
        configuration if the gateway's configuration verifies.
        """

        E = 'EVENT_HANDLER'
        D = 'DISPATCHER'

        if gw_configuration[E] == self.configuration[E] and \
                gw_configuration[D] == self.configuration[D]:
            self.configuration['GATEWAY_ADDRESS'] = tuple(address)
        else:
            self.configuration['GATEWAY_ADDRESS'] = None

        self.gateway_event.set()
        return self.configuration['GATEWAY_ADDRESS'] is not None

class NameService(threading.Thread):
    """ Keeps track of all devices and the gateway in the test.
    """

    logger = Log.get_logger('NameService')

    def __init__(self, address, configuration):
        threading.Thread.__init__(self)
        self.daemon = True
        self.gateway_verification_event = threading.Event()
        self.configuration = configuration

        DeviceType = configuration['DEVICE_TYPE']
        quantity = configuration['DEVICE_QUANTITY']
        frequency = configuration['DEVICE_FREQUENCY']
        delay = configuration['DEVICE_DELAY']
        daddr = ('', 0) # devices on localhost and random port

        self.devices = [DeviceType(daddr, frequency, delay)
                for _ in range(quantity)]
        self.server = TCPServer(
            address,
            NameServiceAPI(
                self.devices,
                configuration,
                self.gateway_verification_event))

    def verify_gateway(self):
        """ Waits for the gateway to connect and be verified. Creates a gateway
        stub if successful.
        """

        self.gateway_verification_event.wait()
        gwaddr = self.configuration['GATEWAY_ADDRESS']

        if gwaddr is None:
            return False

        self.gateway = Stub(gwaddr)
        return True

    def get_gateway_stub(self):
        if self.gateway is None:
            raise AttributeError(
                'Cannot get gateway stub: Gateway is not verified')

        return self.gateway

    def hostname(self):
        return self.server.hostname()

    def stop_devices(self):
        for device in self.devices:
            device.stop()

    def start_devices(self):
        for device in self.devices:
            device.start()

        NameService.logger.debug('Started {} devices', len(self.devices))

    def close(self):
        hostname = self.server.hostname()
        self.stop_devices()
        self.server.close()
        NameService.logger.info('{}: Stop', hostname)

    def run(self):
        NameService.logger.info('{}: Start', self.server.hostname())

        while True:
            self.server.accept()
