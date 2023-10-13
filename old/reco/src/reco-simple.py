#!/usr/bin/env python3
from __future__ import print_function
from threading import Thread
import bottle, sys, math, random
from bottle import static_file
from pyosc import Server, Client

END = '\033[0m'
WHITE = '\033[0;37;48m'
TRANS = '\033[96m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'
BW = '\033[30m\033[47m'

class Reco:
    global is_restart_needed

    def __init__(self, osc_server_port=45001, osc_client_host='127.0.0.1', osc_client_port=53000, http_server_port=8080, size='medium_url', maxwords=1000):
        self.osc_client = Client(osc_client_host, osc_client_port)
        self.osc_server = Server('0.0.0.0', osc_server_port, self.callback)

        self.insertList = ["CRRR", "BOOM", "BANG", "ZUT", "CRACK"]
        self.insertCopy = self.insertList.copy()

        self.http_server_port = http_server_port
        self.silent = True
        self.is_restart_needed = True

        self.http_server = bottle.Bottle()

        self.silent = False
        self.count = 0
        self.sentence = False
        self.splLen = 0
        self.splLenPrev = 0
        self.splFloor = 0
        self.splModulo = 0
        self.splFloorPrev = 0
        self.splModuloPrev = 0
        self.size=size

        self.mess = ""

        self.maxwords = maxwords

        print(END + WHITE)
        print(BW + '*** Open chrome at http://127.0.0.1:%d ***' % self.http_server_port)
        print(END + WHITE)

        self.http_server.get('/', callback=self.index)
        self.http_server.post('/getconfig', callback=self.config)
        self.http_server.post('/result', callback=self.result)
        self.http_server.get('/need_restart', callback=self.need_restart)
        self.http_server.run(host='localhost', port=self.http_server_port, quiet=True)

    def callback(self, address, *args):
        # print('OSC message = "%s"' % message)
        if address == '/record':
            self.silent = False
        elif address == '/pause':
            self.silent = True
        elif address == '/restart':
            self.is_restart_needed = True
            self.silent = False
        elif address == '/size':
            if len(args) >= 1 :
                self.size = str(args[0])
                print (END + WHITE + "-size "+self.size)
        elif address == '/exit':
            self.osc_server.stop()
            self.http_server.close()
            sys.exit(0)
        elif address == '/insertNext':
            self.insertNext()
        elif address == '/insertReset':
            self.insertReset()
        elif address == '/insertRandom':
            self.insertRandom()
        else:
            print("callback : "+str(address))
            for x in range(0,len(args)):
                print("     " + str(args[x]))

    def insertNext(self):
        if(len(self.insertList) == 0):
            self.insertList = self.insertCopy.copy()
        i = self.insertList.pop(0).strip()
        print("INSERT NEXT", i)
        self.mess = self.mess + " " + i
        self.osc_client.send('/cue/reco/text', self.mess)

    def insertReset(self):
        self.insertList = self.insertCopy.copy()
        print("INSERT RESET")

    def insertRandom(self):
        if(len(self.insertList) == 0):
            self.insertList = self.insertCopy.copy()
        idx = random.randrange(len(self.insertList))
        i = self.insertList.pop(idx).strip()
        print("INSERT", i)
        self.mess = self.mess + " " + i
        self.osc_client.send('/cue/reco/text', self.mess)

    def result(self):
        result = {'transcript': bottle.request.forms.getunicode('transcript'),
                'confidence': float(bottle.request.forms.get('confidence', 0)),
                'sentence': int(bottle.request.forms.sentence)}
        self.mess = result['transcript'] #self.mess + " " +

        if self.silent == True:
            if result['sentence'] == 1:
                print("(pause)phras_ " + self.mess)
            else:
                print("(pause)mots _ " + self.mess)
            return {'silent':True}

        self.sentence  = result['sentence'] == 1
        spl = self.mess.split(" ")
        self.splLenPrev = self.splLen
        self.splLen = len(spl)


        self.splFloor = math.floor(self.splLen / self.maxwords)
        self.splModulo = self.splLen % self.maxwords

        #print("MODULO "+str(self.splModulo)+" / "+str(self.splFloor)+" / "+str(self.splLen))

        if result['sentence'] == 1:
            return {'silent':False, 'message':self.mess, 'sentence':self.sentence}

        if self.splModulo == self.splModuloPrev and self.splFloor == self.splFloorPrev and self.sentence == False:
            return {'silent':True}

        if self.splFloorPrev < self.splFloor :
            start = self.splFloorPrev*self.maxwords
            end = start + self.splModuloPrev + 1
            #print("SPLITTED    _ "+str(start)+" - "+str(end))
            spl = spl[start:end]
            self.mess = ''.join(str(e)+" " for e in spl)
            print(END + WHITE + "splitted    -|" + self.mess + "|-")
            if self.mess :
                #self.osc_client.send('/litho/words', mess.upper())
                self.sentence = True
        elif self.sentence :
            start = self.splFloor*self.maxwords
            end = start + self.splModulo + 1
            #print("SENTENCE    _ "+str(start)+" - "+str(end))
            spl = spl[start:end]
            self.mess = ''.join(str(e)+" " for e in spl)
            print(END + WHITE + "phrase      -|" + self.mess + "|-")
            if self.mess :
                self.osc_client.send('/cue/reco/text', self.mess)
        else:
            start = self.splFloor*self.maxwords
            end = start + self.splModulo + 1
            #print("WORDS       _ "+str(start)+" - "+str(end))
            spl = spl[start:end]
            self.mess = ''.join(str(e)+" " for e in spl)
            print(END + WHITE + "mots        -|" + self.mess + "|-")
            if self.mess :
                self.osc_client.send('/cue/reco/text', self.mess)

        self.splFloorPrev = self.splFloor
        self.splModuloPrev = self.splModulo

        tmp = self.mess
        if(self.sentence):
            self.mess = ""

        return {'silent':False, 'message':tmp, 'sentence':self.sentence}

    def need_restart(self):
        if self.is_restart_needed:
            self.is_restart_needed = False
            return 'yes'
        return 'no'

    def index(self):
        return static_file('index2.html',root='')

    def config(self):
        return {'ip':self.osc_client.getIp(), 'max':self.maxwords}


if __name__ == '__main__':
    if len(sys.argv) == 1:
        Reco();
    elif len(sys.argv) == 2:
        Reco(maxwords=int(sys.argv[1]))
    elif len(sys.argv) == 5:
        Lithosys(int(sys.argv[1]), sys.argv[2], int(sys.argv[3]), int(sys.argv[4]))
    else:
        print('usage: %s <osc-server-port> <osc-client-host> <osc-client-port> <http-server-port>')
