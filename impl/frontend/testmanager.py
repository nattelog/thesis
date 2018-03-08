import Queue
import threading
import time
from device import PassiveDevice, NameService
from db import Scenario, EventLifecycle, Configuration
from log import (\
        now,
        Log,
        UDPWriter,
        StandardWriter,
        LogServer)


class TestManager:

    logger = Log.get_logger('TestManager')

    def __init__(self, nsport, configuration):
        self.configuration = configuration
        nsaddr = ('', nsport)
        self.nameservice = NameService(nsaddr, configuration)
        self.scenario = Scenario()
        self.lifecycle = EventLifecycle()
        self.configuration_table = Configuration()
        self.sid = self.scenario.create_scenario()
        self.event_messages = Queue.Queue(100)

    def sync_time(self):
        """ Time must be synced between the gateway and the test manager, since
        they can run on different machines. This time syncing algorithm is
        inspired by the one from
        http://www.mine-control.com/zack/timesync/timesync.html.

        This functions caches the offset time in self.offset and stores it in
        the database.
        """

        gwstub = self.nameservice.get_gateway_stub()
        offsets = []
        TestManager.logger.info('Start time sync')

        for i in range(5): # running too many times will fill log queue
            t0 = now()
            t1 = gwstub.get_timestamp()
            t2 = now()
            latency = int((t2 - t0) / 2)
            offset = t1 - t2 + latency
            offsets.append(offset)
            TestManager.logger.debug('Time sync iteration {}: offset {}', i, offset)
            time.sleep(1)

        offsets.sort()
        self.offset = offsets[len(offsets) / 2] # cache offset for further use

        TestManager.logger.info('Time offset is {} ms', self.offset)
        self.scenario.set_offset_time(self.sid, self.offset)

    def verify_gateway(self, lsaddress):
        nsaddress = self.nameservice.hostname()
        self.nameservice.start()
        TestManager.logger.info(
                'Start gateway with ./gateway -l {}:{} -n {}:{} -d {} -e {} -c {} -i {}',
                lsaddress[0],
                lsaddress[1],
                nsaddress[0],
                nsaddress[1],
                self.configuration['DISPATCHER'],
                self.configuration['EVENT_HANDLER'],
                self.configuration['CPU_INTENSITY'],
                self.configuration['IO_INTENSITY']
                )

        return self.nameservice.verify_gateway()

    def save_configuration(self, key, value):
        self.configuration_table.register_key(self.sid, key, value)

    def start_test(self):
        self.start_time = now()
        self.scenario.set_start_time(self.sid, self.start_time)
        self.nameservice.start_devices()

    def end_test(self):
        self.nameservice.close()
        self.scenario.set_end_time(self.sid, now())
        TestManager.logger.info('Test ended')

    def update_event_lifecycle_time(self, event_keyword, event_id, timestamp):
        rel_time = timestamp - self.start_time

        if event_keyword == 'CREATED':
            self.lifecycle.register_time_created(event_id, self.sid, rel_time)
            return
        if event_keyword == 'FETCHED':
            self.lifecycle.register_time_fetched(event_id, self.sid, rel_time)
            return
        if event_keyword == 'RETRIEVED':
            rel_time -= self.offset
            self.lifecycle.register_time_retrieved(event_id, self.sid, rel_time)
            return
        if event_keyword == 'DISPATCHED':
            rel_time -= self.offset
            self.lifecycle.register_time_dispatched(event_id, self.sid, rel_time)
            return
        if event_keyword == 'DONE':
            rel_time -= self.offset
            self.lifecycle.register_time_done(event_id, self.sid, rel_time)
            return
        else:
            raise AttributeError(
                'Unknown event keyword {}'.format(repr(event_keyword))
                )

    def register_event_message(self, message):
        self.event_messages.put(message, True, 10) # raise if queue is full for 10 sec

    def handle_event_message(self):
        message = self.event_messages.get()
        self.update_event_lifecycle_time(**message)
