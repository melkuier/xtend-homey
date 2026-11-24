'use strict';

const Homey = require('homey');

class XtendDriver extends Homey.Driver {

  async onInit() {
    this.log('Xtend driver initialized');
  }

  // Since the ESP32 bridge isn't discoverable on the network, we simply
  // offer one device to add. The user fills in the ESP32's IP address
  // afterwards in the device settings.
  async onPairListDevices() {
    return [
      {
        name: 'Intergas Xtend',
        data: {
          id: `xtend-${Date.now()}`,
        },
        settings: {
          ip_address: '',
          poll_interval: 60,
        },
      },
    ];
  }

}

module.exports = XtendDriver;
