var ntcore = require('./build/Release/ntcore_node');

var table, sendableChooser, command;

try {
    ntcore.init(ntcore.SERVER);
    table = ntcore.getTable("sample_text");
    table.put("c", 2.5);
    table.onChange("c", global.lis0 = function(k, v){
        console.log("onChange", k, v);
    });
    
    sendableChooser = ntcore.getTable("sample_text/chooser");
    sendableChooser.onChange("selected", global.lis1 = function(k, v){
        console.log("Chooser chose", v);
    });
    sendableChooser.put("options", ["sample text", "option 2", "option 5"]);
    sendableChooser.put("selected", "option 2");
    
    command = ntcore.getTable("sample_text/command");
    command.put("running", false);
    command.onChange("running", global.lis2 = function(k, v){
        console.log("Command running: ", v);
        if(v){
            setTimeout(function(){
                command.put("running", false);
                console.log("Command ended");
            }, 5000);
        }
    });
    
    ntcore.getTable("path/sub/table").put("val", 21);
    
    ntcore.getTable("").put("hi", 42);
    ntcore.getTable("").put("suuuuuuuuuuuuuuuuuuuuuuuuuuuuperlong", 42);
    ntcore.getTable("path/sub").put("suuuuuuuuuuuuuuuuuuuuuuuuuuuuperlong", 42);
    
    console.log(ntcore.getAllEntries());
} catch(e) {
    if (e.stack) {
        console.error(e.stack);
    } else {
        console.error(e);
    }
}

(function wait() {
    setTimeout(wait, 1000);
    table.put("s", table.get("s", 0) + 1);
    table.put("boolean", Math.random() > 0.5);
    table.put("string", "string-" + Math.random());
    table.put("array-number", [Math.random(), Math.random(), Math.random()]);
    table.put("array-boolean", [Math.random()>0.5, Math.random()>0.5, Math.random()>0.5]);
    table.put("array-string", [Math.random()+"sssssss", Math.random()+"sssssssss", Math.random()+"ssssss"]);
    table.put("array-misc", [Math.random()+"", Math.random(), Math.random()]);
})();

(function wait2() {
    setTimeout(wait2, 20);
    
    table.put("graph-spam-test", Math.random());
})();