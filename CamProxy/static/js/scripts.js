var socket = io();

socket.on('connect', function() {
    console.log('Verbunden mit Flask-SocketIO');
    socket.emit('getframe');
});

socket.on('cam_data', function(data) {
    document.getElementById('camera-stream').src = 'data:image/jpeg;base64,' + data;
    socket.emit('getframe');
});

socket.on('disconnect', function() {
    console.log('Verbindung zum Server getrennt');
});