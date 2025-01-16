'''
Ethernet learning switch in Python.

Note that this file currently has the code to implement a "hub"
in it, not a learning switch.  (I.e., it's currently a switch
that doesn't learn.)
'''
import switchyard
from switchyard.lib.userlib import *
import time


def main(net: switchyard.llnetbase.LLNetBase):
    my_interfaces = net.interfaces()
    mymacs = [intf.ethaddr for intf in my_interfaces]

    table = {}

    while True:
        try:
            _, fromIface, packet = net.recv_packet()
        except NoPackets:
            continue
        except Shutdown:
            break
        
        for key, (interf, t) in table.copy().items():
            if (time.time() - t) > 10.0:
                del table[key]
            

        log_debug (f"In {net.name} received packet {packet} on {fromIface}")
        eth = packet.get_header(Ethernet)
        if eth is None:
            log_info("Received a non-Ethernet packet?!")
            return
        if eth.dst in mymacs:
            log_info("Received a packet intended for me")
        elif eth.src not in table:
            table[eth.src] = [fromIface, time.time()]
            log_info(f"Add mac_address:{eth.src}, interface:{fromIface} to the table!")
        else:
            if fromIface == table[eth.src][0]:
                table[eth.src][1] = time.time()
            else:
                table[eth.stc] = [fromIface, time.time()]
                log_info(f"Update interface:{fromIface} of mac_address:{eth.src}")
        if eth.dst not in mymacs:
            if eth.dst in table:
                net.send_packet(table[eth.dst][0], packet)
                log_info(f"Sending packet {packet} to {table[eth.dst][0]}")
            else:
                for intf in my_interfaces:
                    if fromIface!= intf.name:
                        log_info (f"Flooding packet {packet} to {intf.name}")
                        net.send_packet(intf, packet)
                        
        for k,v in table.items():
            print("key:", k, "value:", v)

    net.shutdown()
