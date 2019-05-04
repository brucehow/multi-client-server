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

from time import sleep
# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address = ('localhost', 4444)
print ('Connecting to %s port %s' % server_address)
sock.connect(server_address)

count=0
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

        # Make Move
#    sleep(random.randint(5,30))
        message = "000,MOV,CON,3"
        print(f"Sending move {message}")
        sock.sendall(message.encode())
        while True:
            continue
finally:
    print ('Closing socket')
    sock.close()
