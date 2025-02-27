from scapy.all import *
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad
import zlib
import sys, signal


pc_eth_mac = "00:2b:67:36:70:0f"
frdm_eth_mac = "54:27:8d:24:2a:f2"
aes_key = b"My16byteKey00000"
aes_iv = b"My16byteIV000000"

messages_and_replies = { "No todo lo que es oro reluce...": "...Ni todos los que vagan están perdidos.",
                         "Aún en la oscuridad...":"...brilla una luz.",
                         "¿Qué es la vida?":"Nada más que un breve caminar a la luz del sol.",
                         "No temas a la oscuridad...":"...pues en ella se esconden las estrellas.",
                         "Hasta los más pequeños...":"...pueden cambiar el curso del futuro.",
                         "No digas que el sol se ha puesto...":"...si aún te queda la luna.",
                         "El coraje se encuentra...":"...en los lugares más inesperados.",
                         "No todos los tesoros...":"...son oro y plata.",
                         "Es peligroso...":"...cruzar tu puerta.",
                         "Un mago nunca llega tarde...":"...ni pronto, Frodo Bolsón. Llega precisamente cuando se lo propone.",
                         "Aún hay esperanza...":"...mientras la Compañía permanezca fiel.",
                         "El mundo está cambiando...":"...Siento que algo se avecina.",
                          "Las raíces profundas...":"...no alcanzan las heladas.",
                         "No se puede...":"...pasar.",
                         "Y sobre todo...":"...cuidado con el Anillo.",
                         "De las cenizas, un fuego...":"...se despertará.",
}

def signal_handler(signal, frame):
    print("\nprogram exiting gracefully")
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def pba(byte_array):
    formatted_bytes = ", ".join(f"{byte:02X}" for byte in byte_array)
    print(formatted_bytes)

def encrypt(data, key):
    cipher = AES.new(key, AES.MODE_CBC, aes_iv)
    ciphertext = cipher.encrypt(pad(data, AES.block_size))
    return ciphertext

def decrypt(data, key):
    iv = aes_iv
    cipher = AES.new(key, AES.MODE_CBC, iv)
    plaintext = unpad(cipher.decrypt(data), AES.block_size)
    return plaintext

# Computes the CRC32 on the given data
def computeCRC32(data):
    return zlib.crc32(data)

# look for the interface that has the MAC address we want to use
for iface_name, iface_info in conf.ifaces.items():
    # print(f"Interface: {iface_name}, Index: {iface_info.index}, MAC: {iface_info.mac}, IPv4: {iface_info.ip}, Status: {iface_info.flags}")
    if iface_info.mac == pc_eth_mac:
        # print(f"  - This is the interface we want to use!")
        conf.iface = iface_name
print("Listening...")
try:
    while True:
        # Receive packets with the source MAC address of the FRDM board
        rx_packet = sniff(lfilter=lambda x: x.src == frdm_eth_mac, count=1)
        print("")
        print(">>> >>> Received packet:")
        # rx_packet.show()
        if Dot3 in rx_packet[0]:
            payload_len = rx_packet[0][Dot3].len
            payload = bytes(rx_packet[0][Dot3].payload)
        elif Ether in rx_packet[0]:
            payload_len = rx_packet[0][Ether].type
            payload = bytes(rx_packet[0][Ether].payload)
        else:
            print("Invalid packet")
            continue

        #print(f"payload length: {payload_len}")
        #print(f"payload: {payload}")

        # Extract the CRC integer value from the payload
        packet_crc = int.from_bytes(payload[payload_len-4:payload_len], byteorder='little')
        print(f"CRC32: {packet_crc:08x}")

        # Compute the CRC32 of the payload
        calc_crc = zlib.crc32(payload[:payload_len - 4])
        print(f"Calc CRC32: {calc_crc:08x}")

        if packet_crc != calc_crc:
            print("CRC32 is incorrect!")
            continue

        # Decrypt the data
        decrypted_data = decrypt(payload[:payload_len - 4], aes_key)
        decrypted_data = str(decrypted_data, 'utf-8')
        print(f"Decrypted data: {decrypted_data}")

        if decrypted_data in messages_and_replies:
            reply = messages_and_replies[decrypted_data]
        else:  
            reply = "No comprendo"
        print(f"Reply: {reply}")
        reply_bytes = bytes(reply, 'utf-8')
        # print("Reply bytes:")
        # pba(reply_bytes)

        encrypted_data = encrypt(reply_bytes, aes_key)
        # print("Encrypted reply:")
        # pba(encrypted_data)

        # Compute the CRC32 of the reply payload
        calc_crc = zlib.crc32(encrypted_data)
        # print(f"reply Calc CRC32: {calc_crc:08x}")

        send_payload = encrypted_data + calc_crc.to_bytes(4, byteorder='little')
        # Construct an Ethernet packet with Ethertype (Data lenght) 100
        ether = Ether(dst=frdm_eth_mac, src=pc_eth_mac, type=len(send_payload))
        # Combine the Ethernet header and data
        packet = ether/Raw(load=send_payload)
        # Send the packet
        sendp(packet)


except KeyboardInterrupt:
    print("Exiting...")
