from scapy.all import *
import sys, signal

# Actualizando las direcciones MAC según lo proporcionado
pc_eth_mac = "00:2b:67:36:70:0f"  # MAC de la PC
frdm_eth_mac = "54:27:8d:24:2a:f2"  # MAC del RW612

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

# look for the interface that has the MAC address we want to use
for iface_name, iface_info in conf.ifaces.items():
    if iface_info.mac == pc_eth_mac:
        conf.iface = iface_name
print("Listening ")
try:
    while True:
        # Receive packets with the source MAC address of the RW612
        rx_packet = sniff(lfilter=lambda x: x.src == frdm_eth_mac, count=1)
        print("")
        print(">>> >>> Received packet:")
        if Dot3 in rx_packet[0]:
            payload_len = rx_packet[0][Dot3].len
            payload = bytes(rx_packet[0][Dot3].payload)
        elif Ether in rx_packet[0]:
            payload_len = rx_packet[0][Ether].type
            payload = bytes(rx_packet[0][Ether].payload)
        else:
            print("Invalid packet")
            continue

        # Aquí se eliminó la verificación de CRC32 y el texto plano para respuesta
        decrypted_data = payload[:payload_len]  # Placeholder sin cifrado
        decrypted_data = str(decrypted_data, 'utf-8')  # Convertir bytes a cadena
        print(f"Received data: {decrypted_data}")

        if decrypted_data in messages_and_replies:
            reply = messages_and_replies[decrypted_data]
        else:  
            reply = "No comprendo"
        print(f"Reply: {reply}")

        # Placeholder para enviar la respuesta
        print(f"Sending reply: {reply}")

except KeyboardInterrupt:
    print("Exiting...")