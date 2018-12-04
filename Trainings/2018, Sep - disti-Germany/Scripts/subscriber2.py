import paho.mqtt.client as mqtt
import datetime
import time
from influxdb import InfluxDBClient

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("Machine")
    
def on_message(client, userdata, msg):
    #print("Received a message on topic: " + msg.topic)
    # Use utc as timestamp
    receiveTime=datetime.datetime.utcnow()
    message=msg.payload.decode("utf-8")
    isfloatValue=False

    try:
        # Convert the string to a float so that it is stored as a number in the database
        val = float(message)
        isfloatValue=True
    except:
        print("Could not convert " + message + " to a float value")
        isfloatValue=False

    if isfloatValue:
        print("New reading: " + str(val) + ", received at " + str(receiveTime))
        predicted_value = 0

        json_body = [
        {
            "measurement": "ketchupfactory",
            "tags": {
               "host": "machine01",
               "region": "Germany"
            },
            "time": receiveTime,
               "fields": {
                 "pressure": val,
                 "predicted": predicted_value
               }
            }
        ]

        dbclient.write_points(json_body)

# Set up a client for InfluxDB
dbname = 'ARTIKTraining'
dbclient = InfluxDBClient('localhost', 8086, 'root', 'root',dbname)
dblist = dbclient.get_list_database()
db_found = False
for db in dblist:
  if db['name'] == dbname:
    db_found = True
if not(db_found):
  print('Database' + dbname + ' not found, trying to create')
  dbclient.create_database(dbname)

# Initialize the MQTT client that should connect to the Mosquitto broker
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
connOK=False
while(connOK == False):
    try:
        client.connect("localhost", 1883, 60)
        connOK = True
    except:
        connOK = False
    time.sleep(2)

# Blocking loop to the Mosquitto broker
client.loop_forever()
