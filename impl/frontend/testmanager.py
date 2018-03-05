#!/usr/bin/python

"""
Nameservice to track and manage all simulated devices.
"""

from device import PassiveDevice, NameService
from log import (\
        Log,
        UDPWriter,
        LogServer,
        Scenario)

LOG_SERVER_ADDR = ('', 5001)
NAMESERVICE_ADDR = ('', 5000)
DEVICE_QUANTITY = 2
EVENT_FREQUENCY = 0.1

if __name__ == '__main__':
    writer = UDPWriter(LOG_SERVER_ADDR)
    Log.config(Log.LEVEL_DEBUG, writer)
    ls = LogServer(LOG_SERVER_ADDR)
    ls.start()

    ns = NameService(
            NAMESERVICE_ADDR,
            PassiveDevice,
            DEVICE_QUANTITY,
            EVENT_FREQUENCY)
    #ns.start_devices()

    scenario = Scenario()

    """
    try:
        while True:
            ns.accept()
    except KeyboardInterrupt:
        ns.close()
        ls.close()
    """
