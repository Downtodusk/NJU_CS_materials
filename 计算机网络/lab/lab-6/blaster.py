#!/usr/bin/env python3

import time
from random import randint
import switchyard
from switchyard.lib.address import *
from switchyard.lib.packet import *
from switchyard.lib.userlib import *

class swndEntry:
    def __init__(self, packet):
        self.packet = packet
        self.is_acked = False
        self.is_resend = False


class Blaster:
    def __init__(
            self,
            net: switchyard.llnetbase.LLNetBase,
            blasteeIp,
            num,
            length="100",
            senderWindow="5",
            timeout="300",
            recvTimeout="100"
    ):
        self.net = net
        self.dst_ip = IPv4Address(blasteeIp)
        self.snd_num = int(num)
        self.init_num = int(num)
        self.bytes_len_pld = int(length)
        self.swndlen = int(senderWindow)
        self.swnd = []
        self.timeout = float(timeout) /1000.0
        self.rcv_timeout = float(recvTimeout) /1000.0 #每个 recv_timeout 循环应发送一个数据包
        self.LHS = 1
        self.RHS = 1
        self.inTimeoutResend = 0
        self.lastPacketSend = 0
        self.lhs_flag = True

        self.timeCnt = 0
        self.initTime = time.time()

        self.total_TX_time = 0
        self.num_reTX = 0
        self.num_timeOut = 0
        self.throughput = 0
        self.goodput = 0

    def make_packet(self):
        # Creating the headers for the packet
        pkt = Ethernet() + IPv4() + UDP()
        pkt[1].protocol = IPProtocol.UDP

        # Do other things here and send packet
        pkt[0].ethertype = EtherType.IP     
        pkt[0].src = EthAddr('10:00:00:00:00:01')
        pkt[0].dst = EthAddr('40:00:00:00:00:01')
        pkt[1].src = '192.168.100.1'
        pkt[1].dst = self.dst_ip
        pkt[1].ttl = 64
        
        seq_num = self.RHS.to_bytes(4, "big")
        pkt += seq_num
        pkt += self.bytes_len_pld.to_bytes(2, 'big')
        # data = f"hello world".encode(encoding="UTF-8")
        # data += bytes(self.bytes_len_pld - len(data))
        data = bytes(self.bytes_len_pld)
        pkt += data

        self.swnd.append(swndEntry(pkt))
        self.RHS += 1
        self.throughput += self.bytes_len_pld
        self.goodput += self.bytes_len_pld
        return pkt
    
    def check_send(self):
        for i in range(self.lastPacketSend, len(self.swnd)):
            if self.swnd[i].is_acked == False and self.swnd[i].is_resend == False:
                self.net.send_packet('blaster-eth0', self.swnd[i].packet)
                self.num_reTX += 1
                self.swnd[i].is_resend = True
                self.throughput += self.bytes_len_pld 
                self.lastPacketSend = i
                return False
        return True

    def resendPkt(self):
        oneTurnEnd = self.check_send()
        if oneTurnEnd:
            self.inTimeoutResend -= 1
            self.lastPacketSend = 0
            for it in self.swnd:
                it.is_resend = False
            self.check_send()

    def handle_packet(self, recv: switchyard.llnetbase.ReceivedPacket):
        _, fromIface, packet = recv
        log_debug("I got a packet")
        self.total_TX_time = time.time() - self.initTime
        seq_num = int.from_bytes(packet[3].to_bytes()[:4], 'big')
        if seq_num < self.LHS:
            log_info("Received an old ACK!")
            return None
        # log_info(f"I received an ACK packet from blastee, seqnum: {seq_num}")
        
        self.swnd[seq_num - self.LHS].is_acked = True
        
        if seq_num == self.LHS:
            self.lhs_flag = True
            self.swnd.pop(0)
            self.LHS += 1
        while self.swnd and self.swnd[0].is_acked:
            self.swnd.pop(0)
            self.LHS += 1

        if not self.swnd:
            self.inTimeoutResend = 0

    def handle_no_packet(self):
        log_debug("Didn't receive anything")

        if (self.RHS - self.LHS + 1 < self.swndlen or len(self.swnd) == 0 ) and self.snd_num > 0:
            pkt = self.make_packet()
            self.net.send_packet('blaster-eth0', pkt)
            self.snd_num -= 1
            if self.lhs_flag:
                self.lhs_flag = False
                self.timeCnt = time.time()
        elif self.inTimeoutResend > 0:
            self.resendPkt() 
        if time.time() - self.timeCnt > self.timeout:
            self.num_timeOut += 1
            self.inTimeoutResend += 1
            self.timeCnt = time.time()

    def print_stats(self):
        log_info(f"Total TX time (in seconds): {self.total_TX_time}")
        log_info(f"Number of reTX: {self.num_reTX}")
        log_info(f"Number of coarse TOs: {self.num_timeOut}")
        log_info(f"Throughput (Bps): {self.throughput / self.total_TX_time}")
        log_info(f"Goodput (Bps): {self.goodput / self.total_TX_time}")

    def start(self):
        '''A running daemon of the blaster.
        Receive packets until the end of time.
        '''
        while True:
            # log_info(f"stats: LHS:{self.LHS}, RHS:{self.RHS}, packet to send: {self.snd_num}")
            if self.LHS >= self.init_num + 1:
                break
            try:
                recv = self.net.recv_packet(timeout=self.rcv_timeout)
            except NoPackets:
                self.handle_no_packet()
                continue
            except Shutdown:
                break

            self.handle_packet(recv)

        self.print_stats()
        self.shutdown()

    def shutdown(self):
        self.net.shutdown()


def main(net, **kwargs):
    blaster = Blaster(net, **kwargs)
    blaster.start()