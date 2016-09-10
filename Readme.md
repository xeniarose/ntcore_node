# NetworkTables bindings for node.js #

## API ##

    const ntcore = require('ntcore_node');

Almost all functions may throw errors.

### ntcore.setOptions(options) ###

Sets NetworkTables options. Must be called before `init`

Options:

- `team`: FRC team number. See also [NetworkTable::SetTeam(int)](http://first.wpi.edu/FRC/roborio/release/docs/cpp/classNetworkTable.html#a6764afd9f8244eb1770388f18422a4c2)
- `port`: The port number to run the server/client on. `ntcore.DEFAULT_PORT` contains the default NetworkTables port. See also [NetworkTable::SetPort(int)](http://first.wpi.edu/FRC/roborio/release/docs/cpp/classNetworkTable.html#a54f86e145927aa8416b69cd9f399ebb2)
- `updateInterval`: The periodic update interval. See also [NetworkTable::SetUpdateRate(double)](http://first.wpi.edu/FRC/roborio/release/docs/cpp/classNetworkTable.html#a9dc7b49c7c9d76b0ae80d9c6887c25ce)
- `networkIdentity`: The name that the server/other clients will see this server/client as. See also [NetworkTable::SetNetworkIdentity(string)](http://first.wpi.edu/FRC/roborio/release/docs/cpp/classNetworkTable.html#a4ee6bb1680800204fb3078951928edae)

### ntcore.init(mode, addr) ###

Sets up NetworkTables to either run as a server or connect to a server.

`mode`: One of `ntcore.SERVER` or `ntcore.CLIENT`  
`addr`: The IP address or hostname to connect to, if in client mode

### ntcore.dispose() ###

Closes NetworkTables connections and disposes of all table objects. Any remaining table objects from `ntcore.getTable()` will have undefined behavior.

### ntcore.getTable(name) ###

Gets a network table object with the specified `name`.

### ntcore.getAllEntries() ###

Returns an array of all entries NetworkTables knows about, as an array of full pathnames.

### NetworkTable.get(key, defaultValue) ###

Returns a NetworkTables value for the specified `key` if it exists, otherwise `defaultValue`.

### NetworkTable.put(key, value) ###

Inserts or replaces the NetworkTables value for the specified `key` with `value`.

### NetworkTable.remove(key) ###

Deletes the `key` and associated value from the table, if it exists.

### NetworkTable.getTablePath() ###

Returns the NetworkTables path for this table (eg, what was passed to `ntcore.getTable(name)`).

### NetworkTable.onChange(key, function(key, newValue){}) ###

Adds a change listener for the specified `key`.

## Known Issues ##

- Will not exit cleanly on SIGINT because the exit handler is never called
- No support for raw NetworkTable values

## Building ##

For node.js:

- [Install node-gyp + dependencies](https://github.com/nodejs/node-gyp#installation)
- `node-gyp configure build --python "path to your python 2.7"`
- Find the `ntcore_node.node` file

For nw.js:

- [Install nw-gyp + dependencies](https://github.com/nwjs/nw-gyp#installation)
- Getting nw-gyp to work with VS 2015 on Windows required some black magic that I can't remember how to do right now. On Linux and OSX I don't expect issues.
- `nw-gyp configure build --target=<target nw.js version> --python "path to your python 2.7"`. For Windows, add `--msvs_version=2015`.
- Find the `ntcore_node.node` file