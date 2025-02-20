from scapy.all import Ether, sendp
import time
import struct

# Define the Ethernet frame format
SRC_MAC = "00:2b:67:36:70:0F"  # MAC address of the PC
DEST_MAC = "54:27:8d:24:2a:f2"  # MAC address of the RW612

def create_ethernet_frame(data):
    data_length = len(data)
    if data_length < 48:
        data += b'\x00' * (48 - data_length)  # Ensure the payload is at least 48 bytes
    crc = struct.pack('!I', 0xFFFFFFFF)  # Dummy CRC32 value
    frame = Ether(dst=DEST_MAC, src=SRC_MAC) / struct.pack('!H', data_length) / data / crc
    return frame

def send_ethernet_frame(interface, data):
    frame = create_ethernet_frame(data)
    sendp(frame, iface=interface, verbose=False)  # Set verbose to False to avoid printing

if __name__ == "__main__":
    interface = "Ethernet"  # Change this to your network interface
    message = b"Reading data from the network"
    
    while True:
        # Send a message
        send_ethernet_frame(interface, message)
        print("Packet sent")
        # Wait for 3 seconds
        time.sleep(3)
