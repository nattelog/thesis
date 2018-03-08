import socket
import threading
import time
import re


def now():
    return int(time.time() * 1000)


class UDPWriter():
    """ Writer object that writes messages to UDP socket.
    """

    def __init__(self, address):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.connect(address)

    def write(self, message):
        self.socket.send(message)


class StandardWriter():
    """ Prints to stdout. Not thread safe.
    """

    @staticmethod
    def write(message):
        print(message)


class Log():
    """ Logging client that passes all formatted log messages to a writer
    object.
    """

    LEVEL_DEBUG = 0
    LEVEL_INFO = 1
    LEVEL_ERROR = 2
    level = LEVEL_INFO

    _loggers = {}
    _default_writer = StandardWriter

    def __init__(self, name, writer):
        self.name = name
        self.writer = writer

    @staticmethod
    def config(level=None, default_writer=None):
        if level is not None:
            if level <= Log.LEVEL_ERROR:
                Log.level = level
            else:
                raise IndexError('Level must be <= {}'.format(Log.LEVEL_ERROR))

        if default_writer is not None:
            Log._default_writer = default_writer

    @staticmethod
    def get_logger(name, writer=None):
        """ Returns a new instance of a Log object.
        """

        if name in Log._loggers:
            return Log._loggers[name]

        Log._loggers[name] = Log(name, writer)
        return Log._loggers[name]

    def _write(self, level, message, *args):
        """ Formats a log message and writes it.
        """

        fmsg = '{}:{}:{}:'.format(level, now(), self.name)
        fmsg += message.format(*args)

        if self.writer is None:
            Log._default_writer.write(fmsg)
        else:
            self.writer.write(fmsg)

    def debug(self, message, *args):
        if Log.level < Log.LEVEL_INFO:
            self._write('DEBUG', message, *args)

    def info(self, message, *args):
        if Log.level < Log.LEVEL_ERROR:
            self._write('INFO', message, *args)

    def error(self, message, *args):
        self._write('ERROR', message, *args)


class LogServer(threading.Thread):
    """ UDP server handling incoming log messages from the model. Forwards
    lifecycle events to the testmanager and prints everything to stdout.
    """

    logger = Log.get_logger('LogServer', StandardWriter)
    # pre compile for better performance
    event_msg_regex = re.compile(r'^(\w+):(\d+):(\w+):EVENT_LIFECYCLE_(\w+):([\w\-]+)$')

    def __init__(self, address, testmanager):
        threading.Thread.__init__(self)
        self.daemon = True
        self.testmanager = testmanager
        self.server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server.bind(address)
        LogServer.logger.debug('{}: Start', self.server.getsockname())

    def hostname(self):
        return self.server.getsockname()

    def close(self):
        addr = self.server.getsockname()
        self.server.close()
        LogServer.logger.debug('{}: Stop', addr)

    @staticmethod
    def extract_message(message):
        """ Extracts lifecycle event keywords from the message. It is formatted as follows:
        <level>:<timestamp>:<classname>:<lifecycle_event_keyword>:<event_id>.
        Returns the matched keywords in a dict, or None if no match could be
        made.
        """

        m = LogServer.event_msg_regex.match(message)

        if m:
            return {
                'timestamp': int(m.group(2)),
                'event_keyword': m.group(4),
                'event_id': m.group(5)
            }
        else:
            return None

    def run(self):
        while True:
            message, addr = self.server.recvfrom(1024)
            extr = LogServer.extract_message(message)

            if extr is not None:
                self.testmanager.register_event_message(extr)

            print(message)
