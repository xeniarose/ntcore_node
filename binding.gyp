{
    "targets": [
        {
            "target_name": "ntcore_node",
            "sources": [
                "ntcore_node.cc",
                "NetworkTable_node.cc"
            ],
            "include_dirs": [
                "./",
                "./ntcore/include"
            ],
            "libraries": [
                "../ntcore/native/build/binaries/ntcoreStaticLibrary/x64/ntcore.lib"
            ],
            "cflags": [ "-std=c++11" ],
        }
    ]
}