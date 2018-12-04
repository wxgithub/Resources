import paho.mqtt.client as mqtt
import datetime
import time
import random
from influxdb import InfluxDBClient
from random import uniform
from keras.models import load_model
import numpy as np

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("Machine")
    
def on_message(client, userdata, msg):
    #print("Received a message on topic: " + msg.topic)
    # Use utc as timestamp
    receiveTime=datetime.datetime.utcnow()
    message=msg.payload.decode("utf-8")
    isfloatValue=False
    predicted_value=0

    try:
        # Convert the string to a float so that it is stored as a number in the database
        val = float(message)
        isfloatValue=True
    except:
        print("Could not convert " + message + " to a float value")
        isfloatValue=False

    if isfloatValue:
        print("New reading: " + str(val) + ", received at " + str(receiveTime))

        table_name="ketchupfactory"
        table_list = dbclient.get_list_measurements()
        table_found = False
        for table in table_list:
          if table['name'] == table_name:
            table_found = True

        if not(table_found):
          predicted_value = 0
        else:
          results = dbclient.query("SELECT count(pressure) FROM ketchupfactory")
          points = results.get_points()
          for item in points:
            if item['count'] < 3:
              predicted_value = 0
            else:
              results = dbclient.query("SELECT pressure FROM ketchupfactory ORDER BY time DESC LIMIT 3")
              points = results.get_points()
              items = []
              X = []
              items.insert(0, val)
              for item in points:
                items.insert(0, item['pressure'])
              X.append(items)
              X=np.array(X)
              print("Historical Data to feed the model:")
              print(X)
              print("Predicted Data:")
              classes = model.predict(X)
              predicted_value = int(classes[0][0])
              print(predicted_value)
              print("\n")

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

#Load Keras model for prediction
model = load_model('./keras_trained_model_timeseries.h5')
print('Keras model loaded')

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
