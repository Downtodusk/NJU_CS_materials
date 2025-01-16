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

class ICMPErrorType(Enum):
    NOERROR = 0
    NO_MATCH_ENTRY = 1
    TTL_EXPIRED = 2
    ARP_FAILURE = 3
    UNSUPPORTED_FUNC = 4

class waitEntry:
    def __init__(self, ip, t, live, interface, icmp_error):
        self.ip = ip
        self.sendTime = t
        self.cnt = live
        self.pktlist = []
        self.intf = interface
        self.icmp_error = icmp_error
        self.next = self.pre = self

class waitQueue:
    def __init__(self):
        self.head = waitEntry(None,None,None,None,None)
        self.table = {}
        self.size = 0

    def addItem(self, ip, t, live, interface, pkt, icmp_error):
        entry = waitEntry(ip, t, live, interface, icmp_error)
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

    def packetSending(self, packet, nxt_hop_ip, icmp_error):
        print("====packetSending====")
        selected_entry = self.search_in_fwd_table(nxt_hop_ip)
        if selected_entry is None:
            print("No match in fwd table!!!")
            #case1：无匹配项
            return ICMPErrorType.NO_MATCH_ENTRY
        nextHop = selected_entry['next_hop_addr']
        next_intf = selected_entry['interface']
        if nextHop == IPv4Address('0.0.0.0'):
            nextHop = nxt_hop_ip
        if packet[IPv4].src == IPv4Address('0.0.0.0'):
            packet[IPv4].src = self.net.interface_by_name(next_intf).ipaddr
        packet[Ethernet].src = self.net.interface_by_name(next_intf).ethaddr
        if packet[IPv4].ttl <= 1:
            return ICMPErrorType.TTL_EXPIRED
        packet[IPv4].ttl -= 1
        if nextHop in self.arpTable:
            print("Find entry, send packet directly")
            packet[Ethernet].dst = self.arpTable[nextHop]
            self.net.send_packet(next_intf, packet)
        else: 
            print("Renew the wait queue")
            if self.waitQueue.find(nextHop) == None:#之前没有发送过对nextHop的arp请求
                self.waitQueue.addItem(nextHop,0,0,next_intf, [packet], icmp_error)
            else: 
                self.waitQueue.find(nextHop).pktlist.append(packet)
        return ICMPErrorType.NOERROR

    def ipForwarding(self, ipH, packet): #处理ipv4包
        print("===========IPv4 begin===========")
        print(f"==IPv4==: receive packet from ipaddr:{packet[IPv4].src} to {packet[IPv4].dst}")
        print(f"TTL : {packet[IPv4].ttl}")
        nxt_hop_ip = ipH.dst
        src_ip = ipH.src
        is_icmp_error = False #判断是否为icmp error packet
        icmp = packet.get_header(ICMP)
        if icmp is not None:
            if icmp.icmptype == ICMPType.DestinationUnreachable or \
                        icmp.icmptype == ICMPType.TimeExceeded or \
                        icmp.icmptype == ICMPType.ParameterProblem:
                        print("Received ICMP error message!")
                        is_icmp_error = True
        if nxt_hop_ip in self.myip:
            packet = self.handle_icmp_echo_request(ipH, icmp, packet, is_icmp_error)
            if packet is None:
                return
            nxt_hop_ip = ipH.src
        else:
            nxt_hop_ip = packet[IPv4].dst
        ret = self.packetSending(packet, nxt_hop_ip, is_icmp_error)
        if ret == ICMPErrorType.NO_MATCH_ENTRY:
            print("no matching entries!!!\n")
            if is_icmp_error:
                return 
            pkt = self.general_icmp_error(packet, ICMPType.DestinationUnreachable, 0)
            self.packetSending(pkt, src_ip, False)
            return
        if ret == ICMPErrorType.TTL_EXPIRED:
            print("time to live!!!\n")
            if is_icmp_error:
                return
            pkt = self.general_icmp_error(packet, ICMPType.TimeExceeded, 0)
            self.packetSending(pkt, src_ip, False)
        if ret == ICMPErrorType.NOERROR:
            print("Good job, no error!\n")

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
            print("No match in forwarding table")
            return None
        return selected_entry

    def handle_icmp_echo_request(self, ipH, icmp, packet, error_is_for_me):
        if error_is_for_me:
            print("ICMP error is for me, drop it!!!\n")
            return None
        if icmp is None or (icmp is not None and icmp.icmptype == ICMPType.EchoReply):
            packet = self.general_icmp_error(packet, ICMPType.DestinationUnreachable, 3)      
            return packet     
        elif icmp.icmptype == ICMPType.EchoRequest:
            ep = ICMP() #echo reply
            ep.icmptype = ICMPType.EchoReply
            ep.icmpcode = 0
            ep.icmpdata.data = icmp.icmpdata.data
            ep.icmpdata.identifier = icmp.icmpdata.identifier
            ep.icmpdata.sequence = icmp.icmpdata.sequence
            
            ipp = IPv4() #IPv4 reply header
            ipp.protocol = IPProtocol.ICMP
            ipp.src = ipH.dst
            ipp.dst = ipH.src
            ipp.ttl = 64

            eth = Ethernet()
            eth.ethertype = EtherType.IPv4

            packet = eth + ipp + ep
            return packet
        
    def general_icmp_error(self, origpkt, icmp_type, icmp_code):
        print("====general icmp error====")
        dst_ip = origpkt[IPv4].src
        i = origpkt.get_header_index(Ethernet)
        del origpkt[i]

        # print("delete the ethernet header")
        # pktlen = len(origpkt)

        icmp = ICMP()
        icmp.icmptype = icmp_type
        icmp.icmpcode = icmp_code
        # icmp.icmpdata.origdgramlen = pktlen
        icmp.icmpdata.data = origpkt.to_bytes()[:28]

        print(icmp)

        # print("here!!!")

        ip = IPv4()
        ip.protocol = IPProtocol.ICMP
        ip.dst = dst_ip
        ip.ttl = 64

        print("len of packet:", len(origpkt), "ttl:", ip.ttl)

        ether = Ethernet()
        ether.ethertype = EtherType.IPv4
        pkt = ether + ip + icmp
        return pkt

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
                    print(f"send an arp request packet from {self.net.interface_by_name(temp.intf).ipaddr} to {temp.ip} !")
                    self.net.send_packet(self.net.interface_by_name(temp.intf), arpreqpkt)
                    del_entry = self.waitQueue.delete(temp.ip)
                    self.waitQueue.addItem(del_entry.ip, time.time(), del_entry.cnt + 1, del_entry.intf, del_entry.pktlist, del_entry.icmp_error)
                else:
                    if temp.icmp_error:
                        print("arp failure!!!")
                    else:
                        for pkt in temp.pktlist:
                            icmp_error_pkt = self.general_icmp_error(pkt, ICMPType.DestinationUnreachable, 1)
                            print(f"send an icmp error packet due to arp failure | dst: {temp.ip}")
                            self.packetSending(icmp_error_pkt, pkt[IPv4].src, False)
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