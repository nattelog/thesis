"""
Client model. Highly inspired by the lab series in the course "Distributed
Systems", TDDD25 at Linkoping University. Implements a remote method invocation
model.
"""

import json
import threading
import Queue
import time
import socket
import uuid
from log import Log

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
        Producer.logger.debug('{}: Start', id(self))

    def run(self):
        if self.frequency <= 0:
            return

        while not self.stop_event.wait(1 / self.frequency):
            event = Event()
            self.device.put_event(event)
            Producer.logger.debug('{}: Put event \'{}\'', id(self), event)

    def stop(self):
        self.stop_event.set()
        Producer.logger.debug('{}: Stop', id(self))


class Request(threading.Thread):
    """ Representation of a request coming from the gateway.
    """

    logger = Log.get_logger('Request')

    def __init__(self, owner, conn, addr, delay=0):
        threading.Thread.__init__(self)
        self.owner = owner
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
            result = getattr(self.owner, method)(*args)
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
                Request.logger.debug('{}:Delay {} s', self.addr, self.delay)
                time.sleep(self.delay)

            result = self.process_request(request)
            worker.write(result + '\n')
            worker.flush()
        except Exception as e:
            Request.logger.error('The gateway connection has died: {}: {}',
                    type(e), e);
        finally:
            Request.logger.debug('{}: Close', self.addr)
            self.conn.close()


class Server():
    """ Handles incoming connections and requests on the socket.
    """

    logger = Log.get_logger('Server')

    def __init__(self, address, owner, delay=0):
        self.address = address
        self.owner = owner
        self.delay = delay
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.bind(self.address)
        self.server.listen(5)
        Server.logger.debug('{}: Start', self.hostname())

    def close(self):
        hostname = self.hostname()
        self.server.close()
        Server.logger.debug('{}: Close', hostname)

    def accept(self):
        try:
            conn, addr = self.server.accept()
            Server.logger.debug('{}: Accept', addr)
            req = Request(self.owner, conn, addr, self.delay)
            req.start()
        except socket.error:
            Server.logger.error(socket.error)

    def hostname(self):
        return self.server.getsockname()


class PassiveDevice(threading.Thread):
    """ A passive device is a socket server listening for requests that call
    methods on its event queue, and returns the result.
    """

    logger = Log.get_logger('PassiveDevice')

    def __init__(self, address, port, frequency, delay):
        threading.Thread.__init__(self)
        self.daemon = True
        self.address = (address, port)
        self.device = Device()
        self.server = Server(self.address, self.device, delay)
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

    def __init__(self, devices):
        self.devices = devices

    def hostnames(self):
        return [device.hostname() for device in self.devices]


class NameService:
    """ Keeps track of all devices in the test.
    """

    logger = Log.get_logger('NameService')

    def __init__(self, address, Device, quantity, frequency, delay=0):
        self.quantity = quantity;
        self.devices = [Device('', 0, frequency, delay)
                for _ in range(quantity)]
        self.server = Server(address, NameServiceAPI(self.devices))
        NameService.logger.info('{}: Start', self.server.hostname())

    def stop_devices(self):
        for device in self.devices:
            device.stop()

    def start_devices(self):
        for device in self.devices:
            device.start()

        NameService.logger.debug('Started {} devices', len(self.devices))

    def accept(self):
        self.server.accept()

    def close(self):
        hostname = self.server.hostname()
        self.stop_devices()
        self.server.close()
        NameService.logger.info('{}: Stop', hostname)
