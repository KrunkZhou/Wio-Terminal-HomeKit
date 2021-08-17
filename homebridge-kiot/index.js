"use strict";

var Service, Characteristic;

module.exports = function(homebridge) {
  Service = homebridge.hap.Service;
  Characteristic = homebridge.hap.Characteristic;
  homebridge.registerAccessory("homebridge-kiot", "KIOTSwitch", KIOTSwitch);
}

function KIOTSwitch(log, config) {
  var that = this;
  var express = require('express');
  var app = express();

  this.log = log;
  this.name = config.name;
  this.port = config.port;
  this.onStatus = false;

  this._service = new Service.StatelessProgrammableSwitch(this.name);
  this._service.getCharacteristic(Characteristic.ProgrammableSwitchEvent)
    .on('set', this._setProgrammableSwitchEvent.bind(this));

  this.informationService = new Service.AccessoryInformation();
  this.informationService
    .setCharacteristic(Characteristic.Manufacturer, 'KRUNK.CN')
    .setCharacteristic(Characteristic.Model, 'Wio Ternimal')
    .setCharacteristic(Characteristic.FirmwareRevision, '1.0')
    .setCharacteristic(Characteristic.SerialNumber, 'KIOT-'+this.name.replace(/\s/g, '').toUpperCase());
  
  app.get('/kiot-button/1', function (req, res, next) {
    that._service.setCharacteristic(Characteristic.ProgrammableSwitchEvent, Characteristic.ProgrammableSwitchEvent.SINGLE_PRESS);
    res.sendStatus(200);
  });

  app.get('/kiot-button/2', function (req, res, next) {
    that._service.setCharacteristic(Characteristic.ProgrammableSwitchEvent, Characteristic.ProgrammableSwitchEvent.DOUBLE_PRESS);
    res.sendStatus(200);
  });

  app.get('/kiot-button/3', function (req, res, next) {
    that._service.setCharacteristic(Characteristic.ProgrammableSwitchEvent, Characteristic.ProgrammableSwitchEvent.LONG_PRESS);
    res.sendStatus(200);
  });

  var server = app.listen(this.port, function () {
    that.log('KIOT Listening At ', server.address().address, server.address().port);
  });
}

KIOTSwitch.prototype.getServices = function() {
  return [this.informationService, this._service];
}

KIOTSwitch.prototype._setProgrammableSwitchEvent = function(ProgrammableSwitchEvent, callback) {
  this.log("KIOT Switch - " + this.name + " : " + ProgrammableSwitchEvent);
  callback();
}

