'use strict';

const Homey = require('homey');

class IntergasXtendApp extends Homey.App {

  async onInit() {
    this.log('Intergas Xtend Monitor app is running');
  }

}

module.exports = IntergasXtendApp;
