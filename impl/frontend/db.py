import sqlite3
import uuid


class Database:
    """ Abstract class that exposes database methods used by its concrete
    subclasses.
    """

    PATH = 'db'
    _db = None

    def __init__(self):
        if Database._db is None:
            Database._db = sqlite3.connect(Database.PATH)

        self._execute_schema()
        self.row_factory = sqlite3.Row

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
            create table if not exists scenario(
            sid text primary key,
            offset int,
            start int,
            end int
            );
            """

    def get_scenarios(self):
        return self._execute(
            """
            select * from scenario order by start;
            """
        )

    def create_scenario(self):
        sid = str(uuid.uuid4())

        self._execute(
            """
            insert into scenario (sid) values (?)
            """,
            (sid, )
        )

        return sid

    def set_offset_time(self, sid, time):
        self._execute(
            """
            update scenario set offset=? where sid=?
            """,
            (time, sid)
        )

    def set_start_time(self, sid, time):
        self._execute(
            """
            update scenario set start=? where sid=?
            """,
            (time, sid)
        )

    def set_end_time(self, sid, time):
        self._execute(
            """
            update scenario set end=? where sid=?
            """,
            (time, sid)
        )


class EventLifecycle(Database):
    """ A table that contains all the timestamps for an entire event lifecycle.
    """

    def _schema(self):
        """
        A lifecycle table contains:

        - eid: ID of the event.
        - did (disabled for now): ID of the device the event originated from.
        - sid: ID of the scenario context the event worked in.
        - error: Any error message retrieved for the event.
        - created_time: The time when the event was created on the device.
        - fetched_time: The time when the event left the device.
        - retrieved_time: The time when the event arrived to the gateway.
        - dispatched_time: The time when the event was dispatched to an event
          handler on the gateway.
        - done_time: The time when the event was processed by the event handler.
        """

        return \
            """
            create table if not exists eventlifecycle(
            eid text,
            sid text,
            created_time int,
            fetched_time int,
            retrieved_time int,
            dispatched_time int,
            done_time int,
            primary key (eid, sid),
            foreign key (sid) references scenario(sid)
            );
            """

    def get_events(self, sid):
        return self._execute(
            """
            select * from eventlifecycle where sid=?
            """,
            (sid, )
        )


    def register_time_created(self, eid, sid, time):
        self._execute(
            """
            insert into eventlifecycle (eid, sid, created_time)
            values (?, ?, ?)
            """,
            (eid, sid, time)
        )

    def register_time_fetched(self, eid, sid, time):
        self._execute(
            """
            update eventlifecycle set fetched_time=?
            where eid=? and sid=?
            """,
            (time, eid, sid)
        )

    def register_time_retrieved(self, eid, sid, time):
        self._execute(
            """
            update eventlifecycle set retrieved_time=?
            where eid=? and sid=?
            """,
            (time, eid, sid)
        )

    def register_time_dispatched(self, eid, sid, time):
        self._execute(
            """
            update eventlifecycle set dispatched_time=?
            where eid=? and sid=?
            """,
            (time, eid, sid)
        )

    def register_time_done(self, eid, sid, time):
        self._execute(
            """
            update eventlifecycle set done_time=?
            where eid=? and sid=?
            """,
            (time, eid, sid)
        )


class Configuration(Database):
    """ Contains configuration keys and values for a test.
    """

    def _schema(self):
        """ A Configuration table contain:

        - sid: ID of the scenario having this configuration.
        - key: The name of the configuration key.
        - value: The value of the configuration key.
        """

        return \
        """
        create table if not exists configuration(
        sid text,
        key text,
        value text,
        primary key (sid, key),
        foreign key (sid) references scenario(sid)
        );
        """

    def get_configuration(self, sid):
        return self._execute(
            """
            select * from configuration where sid=?
            """,
            (sid, )
        )

    def register_key(self, sid, key, value):
        self._execute(
            """
            insert into configuration (sid, key, value)
            values (?, ?, ?)
            """,
            (sid, key, value)
        )
