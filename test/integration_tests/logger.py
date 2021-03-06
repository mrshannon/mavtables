#!/usr/bin/env python

from __future__ import (absolute_import, division,
                        print_function, unicode_literals)
from builtins import *

import os
import sys
import signal
from time import sleep
from argparse import ArgumentParser
from threading import Thread
from pymavlink import mavutil


def signal_handler(signal, frame):
    sys.exit(0)


def parse_args():
    parser = ArgumentParser(description='Log incoming MAVLink packets.')
    parser.add_argument('system', type=int, help='system ID')
    parser.add_argument('component', type=int, help='component ID')
    parser.add_argument(
        '--verbose', '-v', action='store_true', help='enable verbosity')
    parser.add_argument(
        '--noheartbeat', action='store_true', help='disable heartbeat')
    parser.add_argument(
        '--udp', action='store', help='UDP address:port to connect to')
    parser.add_argument(
        '--serial', action='store', help='serial port device string')
    args = vars(parser.parse_args())
    if not args['udp'] and not args['serial']:
        print('expected --udp or --serial option')
        sys.exit()
    if args['udp'] and args['serial']:
        print('expected --udp or --serial option, but not both')
        sys.exit()
    return args


def heartbeat(mav):
    while True:
        mav.mav.heartbeat_send(0, 0, 0, 0, 0)
        sleep(60)


def start_heartbeats(mav):
    hbthread = Thread(target=heartbeat, args=(mav,))
    hbthread.daemon = True
    hbthread.start()


def main():
    signal.signal(signal.SIGINT, signal_handler)
    os.environ['MAVLINK20'] = '1'
    mavutil.set_dialect('common')
    args = parse_args()
    if args['udp']:
        mav = mavutil.mavlink_connection('udpout:' + args['udp'],
            source_system=args['system'], source_component=args['component'])
    elif args['serial']:
        mav = mavutil.mavlink_connection(args['serial'], baud=57600,
            source_system=args['system'], source_component=args['component'])
    else:
        sys.exit(1)
    if not args['noheartbeat']:
        start_heartbeats(mav)
    while True:
        msg = mav.recv_match(blocking=True)
        if (args['verbose']):
            msg_string = "{:s} from {:d}.{:d}".format(
                msg.get_type(), msg.get_srcSystem(), msg.get_srcComponent())
            try:
                dest_string = " to {:d}.{:d}".format(
                    msg.target_system, msg.target_component)
                msg_string += dest_string
            except:
                pass
            print(msg_string)
        else:
            print(msg.get_type())
        sys.stdout.flush()


if __name__ == '__main__':
    main()
