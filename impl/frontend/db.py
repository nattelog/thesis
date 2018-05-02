import sqlite3
import uuid
import math

class StdevFunc:
    def __init__(self):
        self.M = 0.0
        self.S = 0.0
        self.k = 1

    def step(self, value):
        if value is None:
            return

        tM = self.M
        self.M += (value - tM) / self.k
        self.S += (value - tM) * (value - self.M)
        self.k += 1

    def finalize(self):
        if self.k < 3:
            return -1

        return math.sqrt(self.S / (self.k - 2))

class Database:
    """ Abstract class that exposes database methods used by its concrete
    subclasses.
    """

    PATH = 'db'
    _db = None

    def __init__(self):
        if Database._db is None:
            Database._db = sqlite3.connect(Database.PATH)

        Database._db.create_aggregate('stdev', 1, StdevFunc)
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

    def get_scenario(self, sid):
        return self._execute(
            """
            select * from scenario where sid=?
            """,
            (sid,),
            True
        )

    def get_scenarios(self, from_time):
        return self._execute(
            """
            select * from scenario
            where start >= ?
            order by start;
            """,
            (from_time, )
        )

    def get_last_scenario(self):
        return self._execute(
            """
            select * from scenario order by start desc;
            """,
            (), True
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

    def _get_stat(self, aggr_func, from_time, to_time, sid):
        val = self._execute(
            """
            select {}({} - {}) from eventlifecycle
            where done_time not null and sid=?
            """.format(aggr_func, to_time, from_time),
            (sid, ),
            True
        )[0]

        return round(val) if val is not None else 0

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

    def count_created_events(self, sid):
        return self._execute(
            """
            select count(eid) from eventlifecycle
            where sid=?
            """,
            (sid, ),
            True
        )[0]

    def count_fetched_events(self, sid):
        return self._execute(
            """
            select count(eid) from eventlifecycle
            where fetched_time not null and sid=?
            """,
            (sid, ),
            True
        )[0]

    def count_retrieved_events(self, sid):
        return self._execute(
            """
            select count(eid) from eventlifecycle
            where retrieved_time not null and sid=?
            """,
            (sid, ),
            True
        )[0]

    def count_dispatched_events(self, sid):
        return self._execute(
            """
            select count(eid) from eventlifecycle
            where dispatched_time not null and sid=?
            """,
            (sid, ),
            True
        )[0]

    def count_processed_events(self, sid):
        return self._execute(
            """
            select count(eid) from eventlifecycle
            where done_time not null and sid=?
            """,
            (sid, ),
            True
        )[0]

    def avg_d0(self, sid):
        return self._get_stat('avg', 'created_time', 'done_time', sid);

    def avg_d1(self, sid):
        return self._get_stat('avg', 'created_time', 'fetched_time', sid);

    def avg_d2(self, sid):
        return self._get_stat('avg', 'retrieved_time', 'done_time', sid);

    def stdev_d0(self, sid):
        return self._get_stat('stdev', 'created_time', 'done_time', sid);

    def stdev_d1(self, sid):
        return self._get_stat('stdev', 'created_time', 'fetched_time', sid);

    def stdev_d2(self, sid):
        return self._get_stat('stdev', 'retrieved_time', 'done_time', sid);

    def min_d0(self, sid):
        return self._get_stat('min', 'created_time', 'done_time', sid);

    def min_d1(self, sid):
        return self._get_stat('min', 'created_time', 'fetched_time', sid);

    def min_d2(self, sid):
        return self._get_stat('min', 'retrieved_time', 'done_time', sid);

    def max_d0(self, sid):
        return self._get_stat('max', 'created_time', 'done_time', sid);

    def max_d1(self, sid):
        return self._get_stat('max', 'created_time', 'fetched_time', sid);

    def max_d2(self, sid):
        return self._get_stat('max', 'retrieved_time', 'done_time', sid);

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

class TestReport(Database):
    def _schema(self):
        return \
        """
        create table if not exists testreport(
        name text,
        sid text,
        primary key (name, sid),
        foreign key (sid) references scenario(sid)
        );
        """

    def register_scenario(self, report_name, sid):
        self._execute(
            """
            insert into testreport (name, sid)
            values (?, ?)
            """,
            (report_name, sid)
        )

    def get_reports(self):
        return self._execute(
            """
            select tr.name, max(s.end) from testreport as tr
            left join scenario as s
            on tr.sid = s.sid
            group by tr.name
            order by s.end desc;
            """
        )

    def get_report(self, name):
        return self._execute(
            """
            select
                tr.sid,
                c1.value,
                c2.value,
                c3.value,
                c4.value,
                c5.value,
                c6.value,
                c7.value,
                c8.value,
                c9.value,
                count(e.created_time),
                count(e.fetched_time),
                count(e.retrieved_time),
                count(e.dispatched_time),
                count(e.done_time),
                avg(e.done_time - e.created_time),
                stdev(e.done_time - e.created_time),
                min(e.done_time - e.created_time),
                max(e.done_time - e.created_time),
                avg(e.fetched_time - e.created_time),
                stdev(e.fetched_time - e.created_time),
                min(e.fetched_time - e.created_time),
                max(e.fetched_time - e.created_time),
                avg(e.done_time - e.retrieved_time),
                stdev(e.done_time - e.retrieved_time),
                min(e.done_time - e.retrieved_time),
                max(e.done_time - e.retrieved_time)
            from testreport as tr
            left join scenario as s
            on tr.sid = s.sid
            left join eventlifecycle as e
            on tr.sid = e.sid
            left join configuration as c1
            on tr.sid = c1.sid
            left join configuration as c2
            on tr.sid = c2.sid
            left join configuration as c3
            on tr.sid = c3.sid
            left join configuration as c4
            on tr.sid = c4.sid
            left join configuration as c5
            on tr.sid = c5.sid
            left join configuration as c6
            on tr.sid = c6.sid
            left join configuration as c7
            on tr.sid = c7.sid
            left join configuration as c8
            on tr.sid = c8.sid
            left join configuration as c9
            on tr.sid = c9.sid
            where
                c1.key = 'TEST_DURATION' and
                c2.key = 'POOL_SIZE' and
                c3.key = 'DISPATCHER' and
                c4.key = 'EVENT_HANDLER' and
                c5.key = 'DEVICE_QUANTITY' and
                c6.key = 'DEVICE_FREQUENCY' and
                c7.key = 'DEVICE_DELAY' and
                c8.key = 'CPU_INTENSITY' and
                c9.key = 'IO_INTENSITY' and
                tr.name=?
            group by s.sid
            order by s.end asc
            """,
            (name,)
        )
