'''
Ethernet learning switch in Python.

Note that this file currently has the code to implement a "hub"
in it, not a learning switch.  (I.e., it's currently a switch
that doesn't learn.)
'''
import switchyard
from switchyard.lib.userlib import *

def print_mac_table(mac_table):
    print("MAC地址                 接口        Traffic_Volume")
    print("--------------------------------------")
    for mac, (interface, count) in mac_table.items():
        print(f"{mac}   {interface}    {count}")
    print("--------------------------------------")

def main(net: switchyard.llnetbase.LLNetBase):
    my_interfaces = net.interfaces()
    mymacs = [intf.ethaddr for intf in my_interfaces]

    table = {}
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
        else:
            if eth.src in table:
                if table[eth.src][0] != fromIface:   #interface info changed
                    table[eth.src][0] = fromIface
                    log_info(f"Update port macAddr:{table[eth.src][0]}, interface:{fromIface}")
            else:    #not in table 
                if len(table) == capacity:    #need to delete
                    table_copy = sorted(table.items(), key=lambda x:x[1][1], reverse=False)
                    del table[table_copy[0][0]]
                table[eth.src] = [fromIface, 0]
                log_info(f"Add macAddr:{eth.src}, interface:{fromIface}")
            if eth.dst in table:
                table[eth.dst][1] += 1
                net.send_packet(table[eth.dst][0], packet)
                log_info(f"Sending packet {packet} to {table[eth.dst]}")
            elif (eth.dst not in table or eth.dst == "ff:ff:ff:ff:ff:ff"):
                for intf in my_interfaces:
                    if fromIface!= intf.name:
                        log_info (f"Flooding packet {packet} to {intf.name}")
                        net.send_packet(intf, packet)
        print_mac_table(table)

    net.shutdown()
