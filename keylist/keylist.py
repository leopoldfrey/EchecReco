import asyncio, socket, evdev
from evdev import InputDevice, categorize, ecodes
from pyosc import Client
from pyosc import Server
from select import select

# GET IP ADDRESS

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(0)
    try:
        # doesn't even have to be reachable
        s.connect(('10.254.254.254', 1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP

class Makeymakey:

    def __init__(self, osc_server_port=14030, osc_client_host='0.0.0.0', osc_client_port=14028):
        self.myIp = str(get_ip())
        self.verbose = False
        print("My IP: "+self.myIp)
        ip = self.myIp.split(".")
        self.osc_client_host = ip[0] + "." + ip[1] + "." + ip[2] + ".255"
        self.osc_client_port = osc_client_port
        print("Broadcast IP: "+self.osc_client_host)
        
        self.oscServer = Server(self.myIp, osc_server_port, self.callback)
        self.oscClient = Client(self.osc_client_host, self.osc_client_port)
        self.oscClient.send("/makeymakey/ip", self.myIp)

        self.devices = [InputDevice(path) for path in evdev.list_devices()]
        for device in self.devices:
            print(device.path, device.name, device.phys)

        #devices = map(InputDevice, ('/dev/input/event0', '/dev/input/event2'))
        self.devicesList = {dev.fd: dev for dev in self.devices}
        #for dev in devices.values(): print(dev)

        while True:
            try:
                r, w, x = select(self.devicesList, [], [])
                for fd in r:
                    for ev in self.devicesList[fd].read():
                        #print(ev)
                        if ev.type == ecodes.EV_KEY:
                            e = str(categorize(ev)).split(', ')
                            #print(e)
                            ee = e[1].split(" ")
                            if int(ee[0]) == 272 :
                                if self.verbose :
                                    print(e[3], "BTN_LEFT")
                                if e[3] == "down" : 
                                    self.oscClient.send("/makeymakey/mouseDown", 0)
                                elif e[3] == "up" :
                                    self.oscClient.send("/makeymakey/mouseUp", 0)
                            elif int(ee[0]) == 273 :
                                if self.verbose :
                                    print(e[2], "BTN_RIGHT")
                                if e[2] == "down" : 
                                    self.oscClient.send("/makeymakey/mouseDown", 1)
                                elif e[2] == "up" :
                                    self.oscClient.send("/makeymakey/mouseUp", 1)
                            elif int(ee[0]) == 274 :
                                if self.verbose :
                                    print(e[2], "BTN_MIDDLE")
                                if e[2] == "down" : 
                                    self.oscClient.send("/makeymakey/mouseDown", 2)
                                elif e[2] == "up" :
                                    self.oscClient.send("/makeymakey/mouseUp", 2)
                            else:
                                cod = ecodes.KEY[int(ee[0])]
                                if self.verbose :
                                    print(e[2], ee[0], cod)
                                if e[2] == "down" : 
                                    self.oscClient.send("/makeymakey/press", (str(ee[0]) + " " +str(cod)))
                                elif e[2] == "up" :
                                    self.oscClient.send("/makeymakey/release", (str(ee[0]) + " " +str(cod)))
                                elif e[2] == "hold" :
                                    self.oscClient.send("/makeymakey/hold", (str(ee[0]) + " " +str(cod)))
                        elif ev.type == ecodes.EV_REL:
                            e = str(categorize(ev)).split(', ')
                            if self.verbose :
                                    print(e[1], ev.value)  
                            if e[1] == "REL_X" :
                                self.oscClient.send("/makeymakey/relX", ev.value)
                            elif e[1] == "REL_Y" :
                                self.oscClient.send("/makeymakey/relY", ev.value)
                            elif e[1] == "REL_WHEEL" :
                                self.oscClient.send("/makeymakey/wheel", ev.value)
            except OSError:
                print("[Errno 19] No such device")
            finally:
                self.devices = [InputDevice(path) for path in evdev.list_devices()]
                #print("DEVICES : "+str(len(self.devices)))
                self.devicesList = {dev.fd: dev for dev in self.devices}

    def callback(self, address, *args):
        if address == "/makeymakey/feed" or address == "/*/feed" :
            print("Client IP : "+args[0])
            self.osc_client_host = args[0]
            self.oscClient = Client(self.osc_client_host, self.osc_client_port)
            self.oscClient.send("/makeymakey/ready", 1)
        elif address == "/makeymakey/test" or address == "/*/test" :
            self.oscClient.send("/makeymakey/ready", 1)
        elif address == "/makeymakey/verbose" or address == "/*/verbose" :
            if int(args[0]) > 0 :
                self.verbose = True
            else:
                self.verbose = False
        elif address == "/makeymakey/devices" :
            self.devices = [InputDevice(path) for path in evdev.list_devices()]
            for device in self.devices:
                self.oscClient.send("/makeymakey/device", str(device.path) + " " + str(device.phys) + " " + str(device.name))
        else:
            print("callback : "+str(address))
            for x in range(0,len(args)):
                print("     " + str(args[x]))

if __name__ == '__main__':
    Makeymakey()
