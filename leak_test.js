var ntcore = require('./build/Release/ntcore_node');

var table;

function randName(l){
	return Array(l).fill(0).map(function(e){
		return "abcdefghijklmnopqrstuvwxyz"[Math.floor(Math.random() * 26)];
	}).join('');
}

try {
    ntcore.init(ntcore.SERVER);
	table = ntcore.getTable("leak_test");
	setInterval(function(){
		var name = randName(10);
		table.put(name, randName(1000000));
		table.remove(name);
	}, 1);
	setInterval(function(){
		global.gc();
	}, 1000);
} catch(e) {
    if (e.stack) {
        console.error(e.stack);
    } else {
        console.error(e);
    }
}