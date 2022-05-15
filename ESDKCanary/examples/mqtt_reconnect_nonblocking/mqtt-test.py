#!/usr/bin/python3

from datetime import datetime
import paho.mqtt.client as paho
import time

broker = "localhost"
port = 1883


# Create function for callback
def on_publish(client, userdata, result):
    print(iso_date)
    pass


# Create client object
client1 = paho.Client("control1")

# Assign function to callback
client1.on_publish = on_publish

# Establish connection
client1.connect(broker, port)

# Publish
while True:
    iso_date = datetime.now().isoformat()
    ret = client1.publish("test/hello", iso_date)
    time.sleep(5)
