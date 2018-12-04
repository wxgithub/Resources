var mqtt = require('mqtt')
var WebSocket = require('ws');

var client  = mqtt.connect('mqtt://localhost')
var webSocketUrl = "";
var deviceid = "";
var token = "";
var ws;

process.argv.forEach(function (val, index, array) {
  if (index == 2) {
    deviceid = val;
    console.log("Device ID" + deviceid);
  } else if (index == 3) {
    token = val;
    console.log("Device Token" + token);
  }
});

if (typeof ws !== 'undefined')
   ws.close();

webSocketUrl = "wss://api.artik.cloud/v1.1/websocket?ack=true";
ws = new WebSocket(webSocketUrl);
ws.on('open', function() {
   register();
});
ws.on('message', function(data) {
   handleRcvAction(data);
});
ws.on('close', function() {
   console.log("WebSocket is closed...");
});

function register() {
    try {
      var registerMessage = '{"type":"register", "sdid":"'+deviceid+'", "Authorization":"bearer '+token+'", "cid":"'+getTimeMillis()+'"}';
      ws.send(registerMessage, {mask: true});
      console.log("Registered");
    }
    catch (e) {
      console.error('Failed to register messages. Error in registering message: ' + e.toString());
    }
}

function senddata(adcdata) {
    try {
      var datamessage = '{"type":"message", "sdid":"'+deviceid+'","cid":"'+getTimeMillis()+'","data":{"sensor":"'+adcdata+'"} }';
      ws.send(datamessage, {mask: true});
    }
    catch (e) {
      console.error('Failed to send data to cloud.' + e.toString());
    }
}

function getTimeMillis(){
   return parseInt(Date.now().toString());
}

function handleRcvAction(data) {
    var sdid = "";
    var msgObj = null;
    if ( data.indexOf("\"type\":\"ping\"") < 0
        && data.indexOf("\"code\":\"200\"") < 0 )
    {
      var msgObj = JSON.parse(data);
      if (msgObj.type)
      {
        if (msgObj.type != "action") {
          return; //Early return;
        } else {
          var ddid = msgObj.ddid; // To identify the device id for the intended device
          var actions = msgObj.data.actions;
          var actionName = actions[0].name; //assume that there is only one action in actions
          console.log("The received action is " + actionName);
          if (msgObj.data)
            console.log(msgObj.data);
        } /* else */
      } /* if */
    } /* if */
 }

client.on('connect', function () {
  client.subscribe('Machine');
  console.log('Subscribed to MQTT broker');
})

client.on('message', function (topic, message) {
  if (ws.readyState == ws.OPEN) {
     console.log('publish sensor data ' + message.toString() + ' to ARTIK Cloud');
     senddata(message);
  } else
     console.log('websocket is not ready');
})
