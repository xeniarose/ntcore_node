#include <node.h>
#include <ntcore.h>
#include <networktables/NetworkTable.h>
#include "NetworkTable_node.h"
#include "ntcore_node.h"

namespace ntcore_node {

using node::AtExit;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Boolean;
using v8::Number;
using v8::Array;
using v8::Value;
using v8::Context;
using v8::Maybe;
using v8::MaybeLocal;
    
bool NTCORE_NODE_DEBUG = true;

void Debug(char* msg){
    if(NTCORE_NODE_DEBUG){
        printf("%s\n", msg);
    }
}

void CreateTable(const FunctionCallbackInfo<Value>& args) {
    Debug("Creating new table");
    NetworkTable_node::NewInstance(args);
}

void Initialize(const FunctionCallbackInfo<Value>& args){
    Debug("ntcore_node: init");
    Isolate* isolate = args.GetIsolate();
     
    if(!args[0]->IsNumber()){
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Mode must be provided")));
        return;
    }
    
    double mode = args[0]->NumberValue();
    if(mode == NTN_MODE_CLIENT){
        NetworkTable::SetClientMode();
        Debug("ntcore_node: Set client mode");
    } else {
        NetworkTable::SetServerMode();
        Debug("ntcore_node: Set server mode");
    }
    
    if(args[1]->IsString()){
        Local<String> v8str = args[1]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* value = *v8StrVal;
        NetworkTable::SetIPAddress(value);
    }
    
    NetworkTable::Initialize();
}
    
void SetOptions(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    Local<Context> ctx = isolate->GetCurrentContext();
    
    if(!args[0]->IsObject()){
        isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Options must be provided")));
        return;
    }
    Local<Object> options = args[0]->ToObject(ctx).ToLocalChecked();
    
    MaybeLocal<Value> team = options->Get(ctx, String::NewFromUtf8(isolate, "team"));
    if(!team.IsEmpty()) {
        Local<Value> val = team.ToLocalChecked();
        if(val->IsNumber()){
            unsigned int teamNum = (unsigned int) (val->NumberValue());
            printf("ntcore_node: team=%d\n", teamNum);
            NetworkTable::SetTeam(teamNum);
        }
    }
    MaybeLocal<Value> port = options->Get(ctx, String::NewFromUtf8(isolate, "port"));
    if(!port.IsEmpty()) {
        // why visual studio, why?!
        #undef SetPort
        Local<Value> val = port.ToLocalChecked();
        if(val->IsNumber()){
            unsigned int portNum = (unsigned int) (val->NumberValue());
            printf("ntcore_node: port=%d\n", portNum);
            NetworkTable::SetPort(portNum);
        }
    }
    MaybeLocal<Value> interval = options->Get(ctx, String::NewFromUtf8(isolate, "updateInterval"));
    if(!interval.IsEmpty()) {
        Local<Value> val = interval.ToLocalChecked();
        if(val->IsNumber()){
            double intervalNum = val->NumberValue();
            printf("ntcore_node: updateInterval=%f\n", intervalNum);
            NetworkTable::SetUpdateRate(intervalNum);
        }
    }
    MaybeLocal<Value> identity = options->Get(ctx, String::NewFromUtf8(isolate, "networkIdentity"));
    if(!identity.IsEmpty()) {
        Local<Value> val = identity.ToLocalChecked();
        if(val->IsString()){
            Local<String> v8str = val->ToString();
            String::Utf8Value v8StrVal(v8str);
            char* value = *v8StrVal;
            printf("ntcore_node: networkIdentity=%s\n", value);
            NetworkTable::SetNetworkIdentity(value);
        }
    }
}
    
void GetAll(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = args.GetIsolate();
    Local<Context> ctx = isolate->GetCurrentContext();
    
    Local<Array> v8Arr = Array::New(isolate);
    uint32_t idx = 0;
    std::shared_ptr<NetworkTable> root = NetworkTable::GetTable("/");
    
    std::vector<nt::EntryInfo> rootInfo = nt::GetEntryInfo("", 0);
    for(auto it = rootInfo.begin(); it != rootInfo.end(); ++it){
        printf("entry: %s\n", it->name.c_str());
        v8Arr->Set(ctx, idx, String::NewFromUtf8(isolate, it->name.c_str()));
        idx++;
    }
    
    args.GetReturnValue().Set(v8Arr);
}
    
void IsConnected(const FunctionCallbackInfo<Value>& args){
    Isolate* isolate = args.GetIsolate();
    Local<Context> ctx = isolate->GetCurrentContext();
    
    bool isConnected = nt::GetConnections().size() > 0;
    
    args.GetReturnValue().Set(Boolean::New(isolate, isConnected));
}

static void Shutdown(void*) {
    Debug("ntcore_node: shutdown");
    NetworkTable::Shutdown();
}

void Dispose(const FunctionCallbackInfo<Value>& args) {
    Isolate* isolate = args.GetIsolate();
    printf("Clearing tables\n");
    NetworkTable_node::networkTables.clear();
    Shutdown(0);
    printf("Disposed\n");
}
    
void Error(const char* location, const char* message){
    printf("Isolate error: %s: %s\n", location, message);
}
    
/*void SignalHandler(uv_signal_t* signalHandle, int signum){
    printf("Got %d, clearing tables\n", signum);
    NetworkTable_node::networkTables.clear();
    Shutdown(0);
    printf("Disposed\n");
}

static uv_signal_t signalHandle_int;
static uv_signal_t signalHandle_hup;*/

void InitAll(Local<Object> exports, Local<Object> module) {
    Isolate* isolate = exports->GetIsolate();
    isolate->SetFatalErrorHandler(&Error);
    Local<Context> ctx = exports->CreationContext();
    
    NetworkTable_node::Init(isolate);
    NODE_SET_METHOD(exports, "init", Initialize);
    NODE_SET_METHOD(exports, "dispose", Dispose);
    NODE_SET_METHOD(exports, "getTable", CreateTable);
    NODE_SET_METHOD(exports, "setOptions", SetOptions);
    NODE_SET_METHOD(exports, "getAllEntries", GetAll);
    NODE_SET_METHOD(exports, "isConnected", IsConnected);

    exports->Set(ctx, String::NewFromUtf8(isolate, "CLIENT"), Number::New(isolate, NTN_MODE_CLIENT));
    exports->Set(ctx, String::NewFromUtf8(isolate, "SERVER"), Number::New(isolate, NTN_MODE_SERVER));
    exports->Set(ctx, String::NewFromUtf8(isolate, "DEFAULT_PORT"), Number::New(isolate, NT_DEFAULT_PORT));
    AtExit(Shutdown);
    
    /*uv_signal_init(uv_default_loop(), &signalHandle_int);
    uv_signal_init(uv_default_loop(), &signalHandle_hup);
    uv_signal_start(&signalHandle_int, &SignalHandler, SIGINT);
    uv_signal_start(&signalHandle_hup, &SignalHandler, SIGHUP);*/
}

NODE_MODULE(ntcore_node, InitAll)

}