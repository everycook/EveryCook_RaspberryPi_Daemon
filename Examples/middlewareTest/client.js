
$(document).ready(function() {
	var log, clearLog, logLines, serverUrl, socket;
	log = function(msg) {
		logLines++;
		if (logLines>500){
			clearLog();
		}
		return $('#log').prepend("" + msg + "<br />");
	};
	
	clearLog = function(){
		$('#log').text('');
		logLines=0;
	};
	
	var ip = $('#server').val();
	var port = $('#port').val();
	var recipeNr = $('#recipeNr').val();
	serverUrl = 'ws://' + ip + ':'+port+'/everycook?recipeNr='+recipeNr;
	
	
	var createConnection = function(serverUrl){
		var socket = null;
		if (window.MozWebSocket) {
			socket = new MozWebSocket(serverUrl);
		} else if (window.WebSocket) {
			socket = new WebSocket(serverUrl);
		}
		socket.binaryType = 'blob';
		socket.onopen = function(msg) {
			return $('#status').removeClass().addClass('online').html('connected');
		};
		socket.onmessage = function(msg) {
			/*
			var response;
			try {
				response = JSON.parse(msg.data);
				log("Action: " + response.action);
				return log("Data: " + response.data);
			} catch (exception){		
				return log("frame: " + msg.data);
			}*/
			return log(msg.data);
		};
		socket.onclose = function(msg) {
			return $('#status').removeClass().addClass('offline').html('disconnected');
		};
		return socket;
	}
	socket = createConnection(serverUrl);
	$('#status').click(function() {
		if($(this).hasClass('online')){
			return socket.close();
		} else {
			var ip = $('#server').val();
			var port = $('#port').val();
			serverUrl = 'ws://' + ip + ':'+port+'/everycook';
			socket = createConnection(serverUrl);
			return socket != null;
		}
	});
	$('#send').click(function() {
		var payload;
		payload = new Object();
		payload.action = $('#action').val();
		payload.data = $('#data').val();
		return socket.send(JSON.stringify(payload));
	});
	$('#sendText').click(function() {
		return socket.send($('#text').val());
	});
	$('#clear').click(function() {
		clearLog();
	});
	return $('#sendfile').click(function() {
		var data, payload;
		data = document.binaryFrame.file.files[0];
		if (data) {
			payload = new Object();
			payload.action = 'setFilename';
			payload.data = $('#file').val();
			socket.send(JSON.stringify(payload));
			socket.send(data);
		}
		return false;
	});
});
