
var vfs = require( '..' );
var server = vfs.WebSocket.Server( { port:8080
	//, perMessageDeflate:false
	, perMessageDeflateAllow:true
	}
);
server.on( "connect", function(ws){
	//console.log( "connected." );
        ws.on( "message", function(msg) {
        	//if( typeof msg == "String" )
        	//console.log( "got message", msg );
                ws.send( msg );
        } );
        ws.on( "close", function() {
        } );
        ws.on( "error", function(err,code) {
        	console.log( "error event (probably not)" );
        } );
} );
