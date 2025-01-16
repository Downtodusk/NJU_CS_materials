#!/usr/bin/env python3

import time
import threading
from struct import pack
import switchyard
from switchyard.lib.address import *
from switchyard.lib.packet import *
from switchyard.lib.userlib import *


class Blastee:
    def __init__(
            self,
            net: switchyard.llnetbase.LLNetBase,
            blasterIp,
            num
    ):
        self.net = net
        self.dst_ip = IPv4Address(blasterIp)
        self.rcv_num = int(num)
        self.rcvCnt = set()

    def handle_packet(self, recv: switchyard.llnetbase.ReceivedPacket):
        _, fromIface, packet = recv
        log_debug(f"I got a packet from {fromIface}")
        log_debug(f"Pkt: {packet}")
        # print(packet)
        eth = Ethernet(src=EthAddr('20:00:00:00:00:01'), dst=EthAddr('40:00:00:00:00:02'))
        ip = IPv4(src=IPAddr('192.168.200.1'), dst=self.dst_ip, protocol=IPProtocol.UDP, ttl=64)
        udp = UDP()
        ACKpkt = eth + ip + udp

        #add sequence number(32 bits)
        packet_data = packet[3].to_bytes()
        decode_seq_num = int.from_bytes(packet_data[:4], 'big')
        encode_seq_num = packet_data[:4]
        ACKpkt += encode_seq_num

        log_info(f"I reveived an packet with sequence number: {decode_seq_num} from blaster")
        # self.rcvCnt.add(decode_seq_num)

        #add payload
        len_of_payload = int.from_bytes(packet_data[4:6], 'big')
        payload_data = packet_data[6:6+len_of_payload]
        if len_of_payload >= 8:
            ACKpkt += payload_data[:8]
        else:
            null_bytes = bytes(8 - len_of_payload)
            ACKpkt += (payload_data + null_bytes)

        #send ACK packet
        self.net.send_packet('blastee-eth0', ACKpkt)

    def start(self):
        '''A running daemon of the blastee.
        Receive packets until the end of time.
        '''
        while True:
            # if len(self.rcvCnt) == 100:
            #     log_info("I've ACKed all packet!")
            #     break
            try:
                recv = self.net.recv_packet(timeout=1.0)
            except NoPackets:
                continue
            except Shutdown:
                break

            self.handle_packet(recv)

        self.shutdown()

    def shutdown(self):
        self.net.shutdown()


def main(net, **kwargs):
    blastee = Blastee(net, **kwargs)
    blastee.start()
