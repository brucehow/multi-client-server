"""
This is a simple example of a client program written in Python.
Again, this is a very basic example to complement the 'basic_server.c' example.


When testing, start by initiating a connection with the server by sending the "init" message outlined in
the specification document. Then, wait for the server to send you a message saying the game has begun.

Once this message has been read, plan out a couple of turns on paper and hard-code these messages to
and from the server (i.e. play a few rounds of the 'dice game' where you know what the right and wrong
dice rolls are). You will be able to edit this trivially later on; it is often easier to debug the code
if you know exactly what your expected values are.

From this, you should be able to bootstrap message-parsing to and from the server whilst making it easy to debug.
Then, start to add functions in the server code that actually 'run' the game in the background.
"""

import socket
import random
import time

from time import sleep

def countdown(t):
    while t:
        timeformat = '{:02d}'.format(t)
        print(timeformat, end='\r')
        time.sleep(1)
        t -= 1

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address = ('localhost', 4444)
print ('Connecting to %s port %s' % server_address)
sock.connect(server_address)

count=0
clientid="";
try:
    while True:
        exit = False

        # Connect to Game Server
        message = "INIT"
        print(f"Connecting to server...")
        sock.sendall(message.encode())

        # Look for the response
        amount_received = 0
        amount_expected = len(message)

        while amount_received < amount_expected:
            data = sock.recv(1024)
            amount_received += len(data)
            mess = data.decode()
            if "WELCOME" in mess:
                print(f"Successfully connected to server!")
                clientid = mess[-3:]
                print(f"Received client ID of {clientid}")
                break
            elif "REJECT" in mess:
                print("Server connection rejected!\nClient terminating...")
                exit = True
                break
            else:
                print (f"Received {mess}, but expected WELCOME or REJECT")
                exit = True
                break
        if exit:
            break

        # Wait for game to start
        print("Waiting for game to start...")
        amount_received = 0
        while amount_received < amount_expected:
            data = sock.recv(1024)
            amount_received += len(data)
            mess = data.decode()
            while True:
                if "START" in mess:
                    print("Game has started")
                    break

        tts = random.randint(7,8);
        while True:
            a = random.randint(0,2);
            moves = ["EVEN", "ODD", "DOUB"]
            message = clientid + ",MOV," + moves[a]
            print(f"-> Sending {moves[a]} in ")
            countdown(tts)

            sock.sendall(message.encode())
            amount_received = 0
            while amount_received < amount_expected:
                data = sock.recv(1024)
                amount_received += len(data)
                mess = data.decode()
                while True:
                    if "FAIL" in mess:
                        print("LOST A LIFE")
                        break
                    if "PASS" in mess:
                        print("MADE IT OK")
                        break

finally:
    print ('Closing socket')
    sock.close()
