/*

    natpeer-server.js

    The rendezvous server

 */

var express  = require("express"),
    mongoose = require("mongoose"),
    util     = require("util"),
    net      = require("net"),
    needle   = require("needle");

var GCM_URL      = "https://android.googleapis.com/gcm/send",
    GCM_AUTH_KEY = "",
    DB_NAME      = "natpeer-server";


function generateID() {
  var id = "";
  for (var i = 0; i < 32; i++)
    id += Math.floor(Math.random() * 16).toString(16).toUpperCase();
  return id;
};

function sendGCMMessage(gcm, message) {
  var options = {
    headers: {
      "Authorization": "key=" + GCM_AUTH_KEY
    }
  }
  var data = "registration_id=" + gcm + "&" +
             "data.message="    + JSON.stringify(message);
  needle.post(GCM_URL, data, options, function(err, res, body) {
    if (err)
      console.error(err);
  });
};

var api = express(),
    Schema = mongoose.Schema;

var Device = new Schema({
  gcm:      { type: String, required: true },
  services: { type: Array, default: [] }
});

var DeviceModel = mongoose.model("Device", Device);

var Service = new Schema({
  name:   { type: String, required: true },
  device: { type: String, required: true }
});

var ServiceModel = mongoose.model("Service", Service);

mongoose.connect("mongodb://localhost/" + DB_NAME);
api.configure(function() {
    api.use(express.bodyParser());
    api.use(express.methodOverride());
    api.use(api.router);
    api.use(express.errorHandler({ dumpExceptions: true, showStack: true }));
});

api.get("/api", function(req, res) {
  res.send(200, { status: "OK" });
});

api.post("/api/devices", function(req, res) {
  var device = new DeviceModel({
    gcm: req.body.gcm,
    services: [],
  });
  device.save(function(err) {
    if (!err)
      return console.log("Device created: " + device._id);
    else
      return console.log(err);
  });
  return res.send(201, device);
});

api.delete("/api/devices/:id", function(req, res) {
  return DeviceModel.findById(req.params.id, function(err, device) {
    if (err || device === null) {
      return res.send(404, { err: "not found" });
    }
    return device.remove(function(err) {
      if (!err) {
        console.log("Device deleted: " + device._id);
      } else {
        console.log(err);
      }
      return res.send(200, { message: "OK" });
    });
  });
});

api.post("/api/services", function(req, res) {
  var service = new ServiceModel({
    name: req.body.name,
    device: req.body.device
  });
  service.save(function(err) {
    if (err) {
      return console.log(err);
    } else {
      return console.log("Service created: " + service.name + " "
        + service._id);
    }
  });
  return res.set("Connection", "close") && res.send(201, service);
});

api.delete("/api/services/:id", function(req, res) {
  return ServiceModel.findById(req.params.id, function(err, service) {
    if (err || service === null) {
      return res.send(404, "{err: 'NOT FOUND'}\n");
    }
    return service.remove(function(err) {
      if (err) {
        console.log(err);
      } else {
        console.log("Service deleted: " + service.name + " " + service._id);
      }
      return res.send(200, { message: "OK" });
    });
  });
});

api.listen(8000);

var requests = {};
var server = net.createServer(function(soc) {

  soc.on("data", function(data) {
    var json;
    try {
      json = JSON.parse(data);
      util.debug(soc.remoteAddress + ":" + soc.remotePort + " " +
                 util.inspect(json));
    } catch (err) {
      util.debug(soc.remoteAddress +":" + soc.remotePort + " " + data);
      soc.end();
      return;
    }

    if (json.event === "request") {
      ServiceModel.findOne({ name: json.service }, function(err, service) {
        if (err) {
          console.log(err);
          return;
        }
        DeviceModel.findById(service.device, function(err, device) {
          if (err) {
            console.log(err);
            return;
          }
          var id = generateID();
          sendGCMMessage(device.gcm, {
            gcm_event: "request",
            id:        id,
            service:   service.name
          });
          requests[id] = {
            client: soc,
            gcm: device.gcm
          };
          if (json.nat) {
            requests[id].clientNat = true;
          }
        });
      });

    } else if (json.event === "response") {

      var clientNat = requests[json.id].clientNat ? 1 : 0;
      var serverNat = json.nat ? 1 : 0;

      requests[json.id].server = soc;
      clientSocket = requests[json.id].client;

      clientSocket.write(JSON.stringify([{
        id        : json.id,
        ip        : clientSocket.remoteAddress,
        port      : clientSocket.remotePort,
        peer_ip   : soc.remoteAddress,
        peer_port : soc.remotePort + serverNat
      }]));

      soc.write(JSON.stringify([{
        id        : json.id,
        ip        : soc.remoteAddress,
        port      : soc.remotePort,
        peer_ip   : clientSocket.remoteAddress,
        peer_port : clientSocket.remotePort + clientNat
      }]));

    } else if (json.event === "connection_info") {
      var server = requests[json.id].server;
      server.write(JSON.stringify({
        id     : json.id,
        isn    : json.isn,
        ts_val : json.ts_val
      }));
      soc.end();
      server.end();
      delete requests[json.id];
    } else {
      soc.end();
    }

  });

  soc.on("end", function() {
    console.log("EOF");
  });

});

server.listen(8001);
