var events = require('events'),
    util = require('util');

/*
 * Main RF24 constructor
 * Process options
 * Tell the board to set it up
 */
var RF24 = function(options) {
    if (!options || !options.board) throw new Error('Must supply required options to NF24');
    this.board = options.board;
    this.board.on('ready', function() {
        console.log('board ready, init RF24', this);
        this._init();
    }.bind(this));

    this.board.on('data', function(message) {
        this._handleMessage(message);
    }.bind(this));
};

util.inherits(RF24, events.EventEmitter);

/*
Command Format
96 commandid
09 pin
00 to_node
255 state
*/
RF24.prototype._command = function(to_node,pin, state) {
    var normalizedPin = this.board.normalizePin(pin);
    var normalizedState = this.board.lpad(3, '0', state);
    var normalizedNode = this.board.lpad(2, '0', to_node);
    var msg = '96' + normalizedPin + normalizedNode + normalizedState;

    this.board.log('info', 'command', msg);
    this.board.write(msg);
};

RF24.prototype._init = function() {
    this._command(0, 0, 0);
};

RF24.prototype.send = function(to_node,pin,state){
    this._command(to_node,pin,state);
}

RF24.prototype._handleMessage = function(message) {
    var splitted = message.slice(0, -1).split('::');
    var err = null;

    if (!splitted.length) {
        return;
    }

    if(splitted[0] != 'rf'){
        return;
    }

    var messageType = splitted[1];     
    
    if(messageType == 'init'){
        this.emit(messageType,null,null);
        return;
    }
    
    var data = {
        state: splitted[2],
        node: splitted[3],
        pin: splitted[4]
    }

    switch(messageType){        
        case 'send': this.emit(messageType,null,data); break;
        case 'fail': this.emit('error',null,data); break;
        case 'recv': this.emit('receive',null,data); break;
    }

};


module.exports = RF24;
