import socket
import threading
import time


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
    """ UDP server handling incoming log messages from the model. Registers all
    messages to the test manager and prints them to stdout.
    """

    logger = Log.get_logger('LogServer', StandardWriter)

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

    def run(self):
        while True:
            message, addr = self.server.recvfrom(1024)
            self.testmanager.register_message(message)
            print(message)
