#!/usr/bin/env python3
from __future__ import print_function
from threading import Thread
import bottle, sys, math, random, threading, time
from bottle import static_file
from pyosc import Server, Client

SLEEP_DUR = 5

END = '\033[0m'
WHITE = '\033[0;37;48m'
TRANS = '\033[96m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'
BW = '\033[30m\033[47m'

class SleepThread(threading.Thread):

    def __init__(self, reco):
        super(SleepThread, self).__init__()
        self._stop = threading.Event()
        self.count = 0
        self.reco = reco

    # function using _stop function
    def stop(self):
        self._stop.set()

    def stopped(self):
        return self._stop.isSet()

    def run(self):
        while self.count < SLEEP_DUR :
            self.count = self.count + 1
            if self.stopped():
                # print("stopped")
                return
            # print("sleeping...", self.count)
            time.sleep(1)
        # print("END OF SLEEP")
        self.reco.osc_client.send('/cue/reco/text', "     ")
        self.reco.osc_client.send('/cue/reco/text/format/fontName', "Avenir")
        self.reco.osc_client.send('/cue/reco/text/format/alignment', "left")
        self.reco.osc_client.send('/cue/reco/text/format/fontSize', 48)
        self.reco.osc_client.send('/cue/reco/text/format/color', [0.95, 1, 0.49, 1])

        self.reco.words = []
        self.reco.lastMess = ""


class Reco:
    global is_restart_needed

    def __init__(self, osc_server_port=45001, osc_client_host='127.0.0.1', osc_client_port=53000, http_server_port=8080, size='medium_url'):
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
        self.size=size

        self.mess = ""
        self.lastMess = ""
        self.words = []

        self.sleepThread = None

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
        elif address == '/insert':
            self.insert(str(args[0]))
        else:
            print("callback : "+str(address))
            for x in range(0,len(args)):
                print("     " + str(args[x]))

    def sleep(self):
        if self.sleepThread :
            self.sleepThread.stop()
        self.sleepThread = SleepThread(self)
        self.sleepThread.start()

    def insert(self, txt):
        print("INSERT", txt)
        self.words.append(txt)
        self.output()

    def insertNext(self):
        if(len(self.insertList) == 0):
            self.insertList = self.insertCopy.copy()
        i = self.insertList.pop(0).strip()
        self.insert(i)

    def insertReset(self):
        self.insertList = self.insertCopy.copy()
        # print("INSERT RESET")

    def insertRandom(self):
        if(len(self.insertList) == 0):
            self.insertList = self.insertCopy.copy()
        idx = random.randrange(len(self.insertList))
        i = self.insertList.pop(idx).strip()
        self.insert(i)

    def result(self):
        result = {'transcript': bottle.request.forms.getunicode('transcript'),
                'confidence': float(bottle.request.forms.get('confidence', 0)),
                'sentence': int(bottle.request.forms.sentence)}
        self.lastMess = self.mess
        self.mess = result['transcript'] #self.mess + " " +
        #print(" >>>>", self.mess, " (LAST", self.lastMess, ")")

        current = self.mess.split(" ")

        if self.lastMess == "" :
            # print("a", len(current), current)
            for a in current :
                if a != "":
                    self.words.append(a)
        elif self.lastMess != self.mess :
            prev = self.lastMess.split(" ")

            if len(current) - len(prev) > 0 :
                # print("b", len(current) - len(prev), current[-1 * (len(current) - len(prev)):])
                for a in current[-1 * (len(current) - len(prev)):] :
                    if a != "":
                        self.words.append(a)
            else:
                # print("c", len(current), current)
                for a in current :
                    if a != "":
                        self.words.append(a)

        self.sentence  = result['sentence'] == 1

        self.output()

        return {'silent':False, 'message':self.mess, 'sentence':self.sentence}

    def output(self):
        self.outmess = ''
        i = 0
        cr = False
        cr2 = False
        cr3 = False
        cr4 = False
        cr5 = False
        cr6 = False
        cr7 = False
        cr8 = False
        for e in self.words :
            i = i + 1
            self.outmess = self.outmess+str(e)+" "
            if len(self.outmess) > 52 and not cr :
                self.outmess = self.outmess+"\n"
                cr = True
            if len(self.outmess) > 2 * 52 and not cr2 :
                self.outmess = self.outmess+"\n"
                cr2 = True
            if len(self.outmess) > 3 * 52 and not cr3 :
                self.outmess = self.outmess+"\n"
                cr3 = True
            if len(self.outmess) > 4 * 52 and not cr4 :
                self.outmess = self.outmess+"\n"
                cr4 = True
            if len(self.outmess) > 5 * 52 and not cr5 :
                self.outmess = self.outmess+"\n"
                cr5 = True
            if len(self.outmess) > 6 * 52 and not cr6 :
                self.outmess = self.outmess+"\n"
                cr6 = True
            if len(self.outmess) > 7 * 52 and not cr7 :
                self.outmess = self.outmess+"\n"
                cr7 = True
            if len(self.outmess) > 8 * 52 and not cr8 :
                self.outmess = self.outmess+"\n"
                cr8 = True

        charnum = len(self.outmess)
        wordnum = len(self.words)

        #print(">>>", self.sentence, wordnum, charnum, self.outmess)

        self.sentences = self.outmess.split("\n")
        if len(self.sentences) > 2 :
            self.outmess = ""
            for j in self.sentences[-2:] :
                self.outmess = self.outmess + j + "\n"

        print("OUT", self.outmess)


        if self.mess :
            self.osc_client.send('/cue/reco/text', self.outmess)
            self.osc_client.send('/cue/reco/text/format/fontName', "Avenir")
            self.osc_client.send('/cue/reco/text/format/alignment', "left")
            self.osc_client.send('/cue/reco/text/format/fontSize', 48)
            self.osc_client.send('/cue/reco/text/format/color', [0.95, 1, 0.49, 1])

        if self.sentence :
            self.words = []
            self.lastMess = ""

        self.sleep()

    def need_restart(self):
        if self.is_restart_needed:
            self.is_restart_needed = False
            return 'yes'
        return 'no'

    def index(self):
        return static_file('index.html',root='')

    def config(self):
        return {'ip':self.osc_client.getIp()}#, 'max':self.maxwords}


if __name__ == '__main__':
    if len(sys.argv) == 1:
        Reco();
    # elif len(sys.argv) == 2:
    #     Reco(maxwords=int(sys.argv[1]))
    elif len(sys.argv) == 5:
        Lithosys(int(sys.argv[1]), sys.argv[2], int(sys.argv[3]), int(sys.argv[4]))
    else:
        print('usage: %s <osc-server-port> <osc-client-host> <osc-client-port> <http-server-port>')
