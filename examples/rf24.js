var arduino = require('../');

var board = new arduino.Board();

board.on('connected', function() {
	console.log('conectado');
	//board.write('Conectado');
});

var nrf = new arduino.RF24({
	board: board
});


nrf.on('init', function() {
	console.log('NRF inicializado');
	nrf.send(3, 2, 0);
});

nrf.on('send', function(err,data) {
	console.log('NRF enviou com sucesso');
	console.log(data);
});

nrf.on('error', function(err,data) {
	console.log('Ocorreu um erro ao enviar dados');
});

nrf.on('receive', function(err,data) {
	console.log('NRF recebeu dados');
	console.log(data);
});
