from ssdpy import SSDPClient
client = SSDPClient()
devices = client.m_search("rpi-esp-monitor")
for device in devices:
    print(device)