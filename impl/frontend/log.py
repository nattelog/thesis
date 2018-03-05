import socket
import sys
import time
import Queue
import threading
import sqlite3


class UDPWriter():
    """ Writer object that writes messages to UDP socket.
    """

    def __init__(self, address):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.connect(address)

    def write(self, message):
        self.socket.send(message)


class StandardWriter():
    """ Prints to stdout.
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
    _default_writer = None

    def __init__(self, name, writer):
        self.name = name
        self.writer = writer
        Log.config()

    @staticmethod
    def config(level=LEVEL_INFO, default_writer=StandardWriter):
        if level <= Log.LEVEL_ERROR:
            Log.level = level
        else:
            raise IndexError('Level must be <= {}'.format(Log.LEVEL_ERROR))

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

        t = repr(int(time.time() * 1000))
        fmsg = '{}:{}:{}:'.format(level, t, self.name)
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
    """ UDP server handling incoming log messages from the model.
    """

    logger = Log.get_logger('LogServer', StandardWriter)

    def __init__(self, address):
        threading.Thread.__init__(self)
        self.daemon = True
        self.server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server.bind(address)
        self.file = open('log.log', 'w')
        LogServer.logger.debug('{}: Start', self.server.getsockname())

    def close(self):
        addr = self.server.getsockname()
        self.server.close()
        self.file.close()
        LogServer.logger.debug('{}: Stop', addr)

    def run(self):
        while True:
            message, addr = self.server.recvfrom(1024)
            print(message)


class Database:
    """ Abstract class that exposes database methods used by its concrete
    subclasses.
    """

    PATH = 'db'
    _db = None

    def __init__(self):
        self._execute_schema()
        self.row_factory = sqlite3.Row

        if Database._db is None:
            Database._db = sqlite3.connect(Database.PATH)

    def close(self):
        if _db is not None:
            _db.close()

    def _schema(self):
        """ This method should be implemented by the concrete subclass
        describing its table schema.
        """

        raise NotImplementedError('Abstract method')

    def _execute_schema(self):
        query = self._schema()
        self._execute(query)

    def _execute(self, query, args = (), one = False):
        """ Executes query with @args and returns the affected rows. If @one is
        True, returns only the top row.
        """

        cur = Database._db.execute(query, args)
        rv = cur.fetchall()
        Database._db.commit()

        return (rv[0] if rv else None) if one else rv


class Scenario(Database):
    """ A table containing information on a test scenario.
    """

    def _schema(self):
        """
        A Scenario table contains:

        - sid: The ID of the scenario.
        - offset: The time offset between the gateway and the device (offs =
          time(gateway) - time(device)).
        - start: The starting time.
        - end: The end time.
        """

        return \
            """
            create table scenario if not exists config(
            sid integer primary key autoincrement,
            offset int,
            start int,
            end int
            );
            """

    def create_scenario(self):
        return self._execute(
            """
            insert into scenario
            """
        )
