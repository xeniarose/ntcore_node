const ntcore = require('./build/Release/ntcore_node');

var chg = function(key, val){
    console.log("onChange: " + key + "=" + val);
};

try {
    ntcore.setOptions({networkIdentity: "node test client"});
    ntcore.init(ntcore.CLIENT, "127.0.0.1");
    
    var table = ntcore.getTable("sample_text");
    //table.onChange("s", chg);
    console.log(ntcore.getAllEntries());
    
    /*table.onChange("array-number", chg);
    table.onChange("array-boolean", chg);
    table.onChange("array-string", chg);
    table.onChange("array-misc", chg);*/
} catch(e){
    if(e.stack){
        console.error(e.stack);
    } else {
        console.error(e);
    }
}

/*setTimeout(function(){
    //table.offChange("s", chg);
    chg = null;
    global.gc();
}, 5000);*/

(function wait () {
    //table.put("c", Math.random());
    console.log(ntcore.isConnected());
    setTimeout(wait, 1000);
})();