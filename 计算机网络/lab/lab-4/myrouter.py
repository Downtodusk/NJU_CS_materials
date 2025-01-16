#!/usr/bin/env python3

'''
Basic IPv4 router (static routing) in Python.
'''

import time
import switchyard
from switchyard.lib.userlib import *
from switchyard.lib.address import *

# show ARP Table information
def showArpTable(table):
    print("----ip addr----|----MAC addr----")
    for key, item in table.items():
        print(f"{key}        {item}")
    print("--------------------------------")

def match(prefix, mask, destaddr):
    return ((int(mask) & int(destaddr)) == (int(prefix) & int(mask)))

def mask_len(mask):
    mask_bin = bin(int(mask))[2:]
    length = mask_bin.count('1')
    return length

class waitEntry:
    def __init__(self, ip, t, live, interface):
        self.ip = ip
        self.sendTime = t
        self.cnt = live
        self.pktlist = []
        self.intf = interface
        self.next = self.pre = self

class waitQueue:
    def __init__(self):
        self.head = waitEntry(None,None,None,None)
        self.table = {}
        self.size = 0

    def addItem(self, ip, t, live, interface, pkt):
        entry = waitEntry(ip, t, live, interface)
        entry.pktlist = pkt
        entry.next = self.head.next
        entry.pre = self.head
        self.head.next.pre = entry
        self.head.next = entry
        self.table[ip] = entry
        self.size += 1

    def top(self):
        if self.size > 0:
            return self.head.pre
        return None
    
    def delete(self, ip):
        if ip not in self.table:
            return None
        self.size -= 1
        t = self.table[ip]
        t.next.pre = t.pre
        t.pre.next = t.next
        self.table.pop(ip)
        return t

    def find(self, ip):
        if ip not in self.table:
            return None
        return self.table[ip]

class Router(object):
    def __init__(self, net: switchyard.llnetbase.LLNetBase):
        self.net = net
        self.interfaces = self.net.interfaces()
        self.myip = [intf.ipaddr for intf in self.interfaces]
        self.mymacs = [intf.ethaddr for intf in self.interfaces] #获取路由器所有端口的MAC地址
        self.arpTable = {}
        self.forwarding_table = []
        self.waitQueue = waitQueue()
        #从路由器自身端口向forwarding_table添加信息
        for intf in self.interfaces:
            self.forwarding_table.append({
                'network_addr':IPv4Address(intf.ipaddr),
                'subnet_addr':IPv4Address(intf.netmask),
                'next_hop_addr':IPv4Address('0.0.0.0'),
                'interface':intf.name
            })
        #从txt文档添加信息
        with open('forwarding_table.txt') as fp:
            for line in fp.readlines():
                buffer = line.split()
                self.forwarding_table.append({
                    'network_addr':IPv4Address(buffer[0]),
                    'subnet_addr':IPv4Address(buffer[1]),
                    'next_hop_addr':IPv4Address(buffer[2]),
                    'interface':buffer[3]  
                })

    def arpHandle(self, arp, ifaceName, packet):
        print("==========arpHandle begin===========")
        print("pre arpTable:")
        showArpTable(self.arpTable)
        if arp.targetprotoaddr in self.myip:
            if arp.operation == ArpOperation.Request:
                self.arpTable[arp.senderprotoaddr] = arp.senderhwaddr
                targFace = self.net.interface_by_ipaddr(arp.targetprotoaddr)
                r_packet = create_ip_arp_reply(targFace.ethaddr,arp.senderhwaddr,arp.targetprotoaddr,arp.senderprotoaddr)
                self.net.send_packet(ifaceName,r_packet)
                print("|handle arp request|\n")
            elif arp.operation == ArpOperation.Reply:
                if arp.senderhwaddr == 'ff:ff:ff:ff:ff:ff' or arp.targethwaddr == 'ff:ff:ff:ff:ff:ff':
                    return None 
                if packet.get_header(Ethernet).dst != self.net.interface_by_name(ifaceName).ethaddr:
                    return None
                else:
                    self.arpTable[arp.senderprotoaddr] = arp.senderhwaddr
                    entry = self.waitQueue.find(arp.senderprotoaddr)
                    if entry == None:
                        return None
                    for pkt in entry.pktlist:
                        pkt[Ethernet].dst = arp.senderhwaddr
                        self.net.send_packet(entry.intf, pkt)
                self.waitQueue.delete(arp.senderprotoaddr)
                print("|handle arp reply|\n")

    def ipForwarding(self, ipH, packet):
        print("===========IPv4 begin===========")
        print(f"==IPv4==: receive packet from ipaddr:{packet[IPv4].src} to {packet[IPv4].dst}")
        ip = packet.get_header(IPv4)
        if ip.dst in self.myip:
            print("target is the router itself, drop it!!!\n")
            return
        else:
            IPv4len = packet[IPv4].total_length
            if IPv4len + 14 != packet.size():
                print("IPv4 length is wrong, drop it!!!\n")
                return
            selected_entry = self.search_in_fwd_table(ip.dst)
            if selected_entry is None:
                return None
            nextHop = selected_entry['next_hop_addr']
            next_intf = selected_entry['interface']
            if nextHop == IPv4Address('0.0.0.0'):
                nextHop = ip.dst
            packet[Ethernet].src = self.net.interface_by_name(next_intf).ethaddr
            packet[IPv4].ttl -= 1
            if nextHop in self.arpTable:
                print("Find entry, send packet directly\n")
                packet[Ethernet].dst = self.arpTable[nextHop]
                self.net.send_packet(next_intf, packet)
            else: 
                print("Renew the wait queue\n")
                if self.waitQueue.find(nextHop) == None:#之前没有发送过对nextHop的arp请求
                    self.waitQueue.addItem(nextHop,0,0,next_intf, [packet])
                else: 
                    self.waitQueue.find(nextHop).pktlist.append(packet)

    def search_in_fwd_table(self, dst_ip):
        maxPre = 0
        selected_entry = {}
        for entry in self.forwarding_table:
            if match(entry['network_addr'], entry['subnet_addr'], dst_ip):
                temp_len = mask_len(entry['subnet_addr'])
                if temp_len > maxPre:
                    maxPre = temp_len
                    selected_entry = entry
        if maxPre == 0:
            print("(CASE 2)No match in forwarding table\n")
            return None
        return selected_entry

    def handle_packet(self, recv: switchyard.llnetbase.ReceivedPacket):
        timestamp, ifaceName, packet = recv

        captureIntf = self.net.interface_by_name(ifaceName)
        eth = packet.get_header(Ethernet)

        arp = packet.get_header(Arp)
        ipH = packet.get_header(IPv4)

        if (eth.dst != EthAddr('ff:ff:ff:ff:ff:ff')) and (eth.dst != captureIntf.ethaddr):
            return None
                
        if arp:
            if eth.ethertype != 2054:
                return None
            self.arpHandle(arp, ifaceName, packet)
        elif ipH:
            self.ipForwarding(ipH, packet)
        else:
            log_info("not an arp or IPv4 packet")

def handle_wait_queue(self):
    len = self.waitQueue.size
    if len == 0:
        return None
    temp = self.waitQueue.top()
    while len > 0:
        if time.time() - temp.sendTime > 1:
            if temp.cnt < 5:
                arpreqpkt = create_ip_arp_request(self.net.interface_by_name(temp.intf).ethaddr, self.net.interface_by_name(temp.intf).ipaddr, temp.ip)
                self.net.send_packet(self.net.interface_by_name(temp.intf), arpreqpkt)
                del_entry = self.waitQueue.delete(temp.ip)
                self.waitQueue.addItem(del_entry.ip, time.time(), del_entry.cnt + 1, del_entry.intf, del_entry.pktlist)
            else:
                self.waitQueue.delete(temp.ip)
        len -= 1
        temp = temp.pre

    def start(self):
        '''A running daemon of the router.
        Receive packets until the end of time.
        '''
        while True:
            self.handle_wait_queue()
            try:
                recv = self.net.recv_packet(timeout=1.0)
            except NoPackets:
                continue
            except Shutdown:
                break
            self.handle_packet(recv)
        self.stop()

    def stop(self):
        self.net.shutdown()


def main(net):
    '''
    Main entry point for router.  Just create Router
    object and get it going.
    '''
    router = Router(net)
    router.start()