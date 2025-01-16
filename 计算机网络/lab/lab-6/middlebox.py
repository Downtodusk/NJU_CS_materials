#!/usr/bin/env python3

import time
import threading
import random
from random import randint

import switchyard
from switchyard.lib.address import *
from switchyard.lib.packet import *
from switchyard.lib.userlib import *


class Middlebox:
    def __init__(
            self,
            net: switchyard.llnetbase.LLNetBase,
            dropRate="0.19"
    ):
        self.net = net
        self.dropRate = float(dropRate)
        self.rcvPkt = 0
        self.lostPkt = 0

    def handle_packet(self, recv: switchyard.llnetbase.ReceivedPacket):
        _, fromIface, packet = recv
        if packet[Ethernet].ethertype != EtherType.IP:
            log_info("Not an IP packet, drop it!")
            return
        if fromIface == "middlebox-eth0":
            log_debug("Received from blaster")
            '''s
            Received data packet
            Should I drop it?
            If not, modify headers & send to blastee
            '''
            self.rcvPkt += 1
            # print(packet)
            if random.random() < self.dropRate:
                log_info(f"Oops : packet lost :(  ")
                self.lostPkt += 1
                return None
            #修改头部信息
            packet[Ethernet].src = EthAddr('40:00:00:00:00:02')
            packet[Ethernet].dst = EthAddr('20:00:00:00:00:01')
            self.net.send_packet("middlebox-eth1", packet)
        elif fromIface == "middlebox-eth1":
            log_debug("Received from blastee")
            '''
            Received ACK
            Modify headers & send to blaster. Not dropping ACK packets!
            net.send_packet("middlebox-eth0", pkt)
            '''
            #修改头部信息
            packet[Ethernet].src = EthAddr('40:00:00:00:00:01')
            packet[Ethernet].dst = EthAddr('10:00:00:00:00:01')
            self.net.send_packet("middlebox-eth0", packet)
        else:
            log_debug("Oops :))")

    def start(self):
        '''A running daemon of the router.
        Receive packets until the end of time.
        '''
        while True:
            try:
                recv = self.net.recv_packet(timeout=1.0)
            except NoPackets:
                continue
            except Shutdown:
                break

            self.handle_packet(recv)

        log_info(f"received {self.rcvPkt} from blaster and lost {self.lostPkt} of them\n dropRate(real):{self.lostPkt/self.rcvPkt}")
        self.shutdown()

    def shutdown(self):
        self.net.shutdown()


def main(net, **kwargs):
    middlebox = Middlebox(net, **kwargs)
    middlebox.start()
