import Queue
import re
import threading
from util import now
from device import PassiveDevice, NameService
from log import (\
        Log,
        UDPWriter,
        StandardWriter,
        LogServer,
        Scenario,
        EventLifecycle)


Log.config(level=Log.LEVEL_DEBUG)

LOG_SERVER_PORT = 5001
NAMESERVICE_PORT = 5000
DEVICE_QUANTITY = 2
DEVICE_DELAY = 0
EVENT_FREQUENCY = 0.1


class TestManager:
    logger = Log.get_logger('TestManager', StandardWriter)

    def __init__(self, nsport, DeviceType, dquan, efreq, ddelay):
        self.gateway_event = threading.Event()
        self.nameservice = NameService(
                ('', nsport),
                DeviceType,
                dquan,
                efreq,
                ddelay,
                self.gateway_event)
        self.scenario = Scenario()
        self.lifecycle = EventLifecycle()
        self.sid = self.scenario.create_scenario()
        self.queue = Queue.Queue(10)

    def start_test(self):
        self.nameservice.start()
        TestManager.logger.info('Waiting for gateway...')
        self.gateway_event.wait()
        self.scenario.set_start_time(self.sid, now())
        self.nameservice.start_devices()

    def end_test(self):
        self.nameservice.close()
        self.scenario.set_end_time(self.sid, now())

    def event_create(self, eid, time):
        self.lifecycle.register_time_created(eid, self.sid, time)

    def event_fetched(self, eid, time):
        self.lifecycle.register_time_fetched(eid, self.sid, time)

    def event_retrieved(self, eid, time):
        self.lifecycle.register_time_retrieved(eid, self.sid, time)

    def event_dispatched(self, eid, time):
        self.lifecycle.register_time_dispatched(eid, self.sid, time)

    def event_done(self, eid, time):
        self.lifecycle.register_time_done(eid, self.sid, time)

    def update_db(self, event_keyword, event_id, timestamp):
        if event_keyword == 'CREATED':
            return self.event_create(event_id, timestamp)
        if event_keyword == 'FETCHED':
            return self.event_fetched(event_id, timestamp)
        if event_keyword == 'RETRIEVED':
            return self.event_retrieved(event_id, timestamp)
        if event_keyword == 'DISPATCHED':
            return self.event_dispatched(event_id, timestamp)
        if event_keyword == 'DONE':
            return self.event_done(event_id, timestamp)
        else:
            raise AttributeError(
                'Unknown event keyword {}'.format(repr(event_keyword))
                )

    @staticmethod
    def extract_message(message):
        """ Extracts lifecycle event keywords from the message. It is formatted as follows:
        <level>:<timestamp>:<classname>:<lifecycle_event_keyword>:<event_id>.
        Returns the matched keywords in a dict, or None if no match could be
        made.
        """

        m = re.match(r'^(\w+):(\d+):(\w+):EVENT_LIFECYCLE_(\w+):([\w\-]+)$', message)

        if m:
            timestamp = m.group(2)
            lifecycle_event = m.group(4)
            eid = m.group(5)

            return {
                'timestamp': m.group(2),
                'event_keyword': m.group(4),
                'event_id': m.group(5)
            }
        else:
            return None

    def register_message(self, message):
        self.queue.put(message, True, 10) # raise if queue is full for 10 sec

    def listen_for_message(self):
        message = self.queue.get()
        extr = TestManager.extract_message(message)

        if extr is not None:
            self.update_db(**extr)


if __name__ == '__main__':
    tm = TestManager(
            NAMESERVICE_PORT,
            PassiveDevice,
            DEVICE_QUANTITY,
            EVENT_FREQUENCY,
            DEVICE_DELAY)
    ls = LogServer(('', LOG_SERVER_PORT), tm)

    writer = UDPWriter(('', LOG_SERVER_PORT))
    Log.config(default_writer=writer)

    ls.start()
    tm.start_test()

    try:
        while True:
            tm.listen_for_message()
    except KeyboardInterrupt:
        tm.end_test()
        ls.close()
