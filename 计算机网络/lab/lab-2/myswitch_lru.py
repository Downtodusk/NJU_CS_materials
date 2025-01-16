'''
Ethernet learning switch in Python.

Note that this file currently has the code to implement a "hub"
in it, not a learning switch.  (I.e., it's currently a switch
that doesn't learn.)
'''
import switchyard
from switchyard.lib.userlib import *

def print_mac_table(mac_table):
    print("MAC地址                 接口        Age")
    print("--------------------------------------")
    for mac, (interface, count) in mac_table.items():
        print(f"{mac}   {interface}    {count}")
    print("--------------------------------------")



def main(net: switchyard.llnetbase.LLNetBase):
    my_interfaces = net.interfaces()
    mymacs = [intf.ethaddr for intf in my_interfaces]

    table = {}  #defalut capacity = 5
    capacity = 5

    while True:
        try:
            _, fromIface, packet = net.recv_packet()
        except NoPackets:
            continue
        except Shutdown:
            break

        log_debug (f"In {net.name} received packet {packet} on {fromIface}")
        eth = packet.get_header(Ethernet)
        if eth is None:
            log_info("Received a non-Ethernet packet?!")
            return
        if eth.dst in mymacs:
            log_info("Received a packet intended for me")
        if eth.src not in table:
            if len(table) >= capacity: #lru out
                table_copy = sorted(table.items(), key = lambda item : item[1][1], reverse=True)
                log_info(f"delete macAddr:{table_copy[0][0]}, interface:{table_copy[0][1][0]}")
                del table[table_copy[0][0]]
            for key, [v1, v2] in table.items():
                table[key][1] += 1
            table[eth.src] = [fromIface, 1]
            log_info(f"Add macAddr:{eth.src}, interface:{fromIface}, age = 1")
        elif eth.src in table:
            if table[eth.src][0] != fromIface:
                table[eth.src][0] = fromIface
                log_info(f"Update port mac_address:{eth.src}, interface:{fromIface}")
            else:
                pass
        else:
            pass
        if eth.dst not in mymacs:
            if eth.dst in table:
                for key, [v1, v2] in table.items():
                    table[key][1] += 1
                table[eth.dst][1] = 1
                net.send_packet(table[eth.dst][0], packet)
            else:
                for intf in my_interfaces:
                    if fromIface != intf.name:
                        log_info (f"Flooding packet {packet} to {intf.name}")
                        net.send_packet(intf, packet)
        print_mac_table(table)

    net.shutdown()
