#include "NetworkTable_node.h"
#include <cstring>
#include "ntcore_node.h"

namespace ntcore_node {
    using v8::Function;
    using v8::FunctionCallbackInfo;
    using v8::FunctionTemplate;
    using v8::Isolate;
    using v8::Local;
    using v8::Number;
    using v8::Object;
    using v8::Array;
    using v8::Boolean;
    using v8::Persistent;
    using v8::String;
    using v8::Value;
    using v8::Persistent;
    using v8::Context;
    using v8::HandleScope;
    using v8::MaybeLocal;
    
    Isolate* NetworkTable_node::p_defaultIsolate = NULL;
    std::queue<PendingUpdate> NetworkTable_node::pendingUpdates;
    std::mutex NetworkTable_node::pendingUpdatesLock;
    uv_async_t NetworkTable_node::asyncHandle;
    std::vector<NetworkTable_node*> NetworkTable_node::networkTables;
    
    Persistent<Function> NetworkTable_node::constructor;
    NetworkTable_node::NetworkTable_node(char* name) {
        mp_name = new char[strlen(name) + 1];
        strcpy(mp_name, name);
        mp_table = NetworkTable::GetTable(mp_name);
        mp_table->AddTableListener(this);
        networkTables.push_back(this);
    }

    NetworkTable_node::~NetworkTable_node() {
        delete mp_name;
        if(mp_table.get() != NULL){
            mp_table->RemoveTableListener(this);
        }
    }
    
    void NetworkTable_node::Init(Isolate* isolate) {
        // Prepare constructor template
        Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
        tpl->SetClassName(String::NewFromUtf8(isolate, "NetworkTable"));
        tpl->InstanceTemplate()->SetInternalFieldCount(1);
        
        // Prototype
        //NODE_SET_PROTOTYPE_METHOD(tpl, "test", Test);
        NODE_SET_PROTOTYPE_METHOD(tpl, "get", Get);
        NODE_SET_PROTOTYPE_METHOD(tpl, "put", Put);
        NODE_SET_PROTOTYPE_METHOD(tpl, "remove", Remove);
        NODE_SET_PROTOTYPE_METHOD(tpl, "onChange", AddListener);
        NODE_SET_PROTOTYPE_METHOD(tpl, "offChange", RemoveListener);
        NODE_SET_PROTOTYPE_METHOD(tpl, "getTablePath", GetName);
        
        constructor.Reset(isolate, tpl->GetFunction());
        
        p_defaultIsolate = isolate;
        uv_async_init(uv_default_loop(), &asyncHandle, &RunCallbackAsync);
    }

    void NetworkTable_node::New(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();

        if (args.IsConstructCall()) {
            // Invoked as constructor: `new MyObject(...)`
            Local<String> v8str = args[0]->ToString();
            String::Utf8Value v8StrVal(v8str);
            char* value = *v8StrVal;
            NetworkTable_node* obj = new NetworkTable_node(value);
            obj->Wrap(args.This());
            args.GetReturnValue().Set(args.This());
        } else {
            // Invoked as plain function `MyObject(...)`, turn into construct call.
            const int argc = 1;
            Local<Value> argv[argc] = { args[0] };
            Local<Function> cons = Local<Function>::New(isolate, constructor);
            args.GetReturnValue().Set(cons->NewInstance(argc, argv));
        }
    }
    
    void NetworkTable_node::NewInstance(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        
        if(!args[0]->IsString()){
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Table name must be provided")));
            return;
        }
        
        const unsigned argc = 1;
        Local<Value> argv[argc] = { args[0] };
        Local<Function> cons = Local<Function>::New(isolate, constructor);
        Local<Object> instance = cons->NewInstance(argc, argv);
        
        args.GetReturnValue().Set(instance);
    }
    
    void NetworkTable_node::GetName(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, obj->mp_name));
    }
    
    void NetworkTable_node::Get(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        if(!args[0]->IsString()){
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Key name must be provided")));
            return;
        }
        
        Local<String> v8str = args[0]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* key = *v8StrVal;
        
        Local<Value> defaultValue = args[1];
        
        std::shared_ptr<nt::Value> ntValue = obj->mp_table->GetValue(key);
        if(ntValue.get() == NULL){
            args.GetReturnValue().Set(defaultValue);
            return;
        }
        
        if(ntValue->IsBoolean()){
            args.GetReturnValue().Set(Boolean::New(isolate, ntValue->GetBoolean()));
        } else if(ntValue->IsDouble()){
            args.GetReturnValue().Set(Number::New(isolate, ntValue->GetDouble()));
        } else if(ntValue->IsString()){
            args.GetReturnValue().Set(String::NewFromUtf8(isolate, ntValue->GetString().str().c_str()));
        } else if(ntValue->IsBooleanArray() || ntValue->IsDoubleArray() || ntValue->IsStringArray()) {
            Local<Array> v8Arr = Array::New(isolate);
            if(ntValue->IsBooleanArray()){
                llvm::ArrayRef<int> ntArr = ntValue->GetBooleanArray();
                for(uint32_t i = 0; i < ntArr.size(); i++){
                    v8Arr->Set(isolate->GetCurrentContext(), i, Boolean::New(isolate, ntArr[i]));
                }
            }
            if(ntValue->IsDoubleArray()){
                llvm::ArrayRef<double> ntArr = ntValue->GetDoubleArray();
                for(uint32_t i = 0; i < ntArr.size(); i++){
                    v8Arr->Set(isolate->GetCurrentContext(), i, Number::New(isolate, ntArr[i]));
                }
            }
            if(ntValue->IsStringArray()){
                llvm::ArrayRef<std::string> ntArr = ntValue->GetStringArray();
                for(uint32_t i = 0; i < ntArr.size(); i++){
                    v8Arr->Set(isolate->GetCurrentContext(), i, String::NewFromUtf8(isolate, ntArr[i].c_str()));
                }
            }
            
            args.GetReturnValue().Set(v8Arr);
        } else {
            args.GetReturnValue().Set(defaultValue);
        }
    }
    
    void NetworkTable_node::Put(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        if(!args[0]->IsString() || args[1]->IsUndefined() || args[1]->IsNull()) {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Key name and value must be provided")));
            return;
        }
        
        Local<String> v8str = args[0]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* key = *v8StrVal;
        
        Local<Value> v8Value = args[1];
        
        if(v8Value->IsBoolean()){
            obj->mp_table->PutBoolean(key, v8Value->BooleanValue());
        } else if(v8Value->IsNumber()){
            obj->mp_table->PutNumber(key, v8Value->NumberValue());
        } else if(v8Value->IsString()){
            Local<String> v8ValueStr = v8Value->ToString();
            String::Utf8Value v8ValueUtf8(v8ValueStr);
            obj->mp_table->PutString(key, *v8ValueUtf8);
        } else if(v8Value->IsArray()) {
            Local<Array> v8Arr = Local<Array>::Cast(v8Value);
            if(v8Arr->Length() == 0){
                return;
            }
            Local<Value> first = v8Arr->Get(isolate->GetCurrentContext(), 0).ToLocalChecked();
            if(first->IsBoolean()){
                std::vector<int> ntArr;
                for(uint32_t i = 0; i < v8Arr->Length(); i++){
                    MaybeLocal<Value> maybe = v8Arr->Get(isolate->GetCurrentContext(), i);
                    if(maybe.IsEmpty()){
                        continue;
                    }
                    Local<Value> local = maybe.ToLocalChecked();
                    if(local->IsBoolean()){
                        ntArr.push_back(local->BooleanValue());
                    }
                }
                obj->mp_table->PutBooleanArray(key, ntArr);
            } else if(first->IsNumber()){
                std::vector<double> ntArr;
                for(uint32_t i = 0; i < v8Arr->Length(); i++){
                    MaybeLocal<Value> maybe = v8Arr->Get(isolate->GetCurrentContext(), i);
                    if(maybe.IsEmpty()){
                        continue;
                    }
                    Local<Value> local = maybe.ToLocalChecked();
                    if(local->IsNumber()){
                        ntArr.push_back(local->NumberValue());
                    }
                }
                obj->mp_table->PutNumberArray(key, ntArr);
            } else if(first->IsString()){
                std::vector<std::string> ntArr;
                for(uint32_t i = 0; i < v8Arr->Length(); i++){
                    MaybeLocal<Value> maybe = v8Arr->Get(isolate->GetCurrentContext(), i);
                    if(maybe.IsEmpty()){
                        continue;
                    }
                    Local<Value> local = maybe.ToLocalChecked();
                    if(local->IsString()){
                        String::Utf8Value localUtf8(local->ToString());
                        ntArr.push_back(std::string(*localUtf8));
                    }
                }
                obj->mp_table->PutStringArray(key, ntArr);
            } else {
                isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Array value type not supported. Use an array of string, boolean, or number")));
            }
        } else {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Value type not supported. Use a string, boolean, number, or array of those")));
        }
    }
    
    void NetworkTable_node::Remove(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        if(!args[0]->IsString()) {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Key name must be provided")));
            return;
        }
        
        Local<String> v8str = args[0]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* key = *v8StrVal;
        
        obj->mp_table->Delete(key);
    }
    
    void NetworkTable_node::AddListener(const FunctionCallbackInfo<Value>& args) {
        std::lock_guard<std::mutex> _lock(pendingUpdatesLock);
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        if(!args[0]->IsString() || !args[1]->IsFunction()) {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Key name and callback must be provided")));
            return;
        }
        
        Local<String> v8str = args[0]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* key = *v8StrVal;
        
        std::string keyStr = key;
        cb_vec_t& vec = obj->m_callbacks[keyStr];
        
        cb_persist_t funcHandle(isolate, Local<Function>::Cast(args[1]));
        vec.push_back(funcHandle);
        
        CbWeakCallbackData* data = new CbWeakCallbackData;
        data->key = std::string(key);
        data->parent = obj;
        
        vec.at(vec.size() - 1).SetWeak(data, CbWeakCallback);
        vec.at(vec.size() - 1).MarkIndependent();
    }
    
    void NetworkTable_node::CbWeakCallback(const v8::WeakCallbackData<v8::Function, CbWeakCallbackData>& data){
        v8::Isolate* isolate = data.GetIsolate();
        v8::HandleScope scope(isolate);
        CbWeakCallbackData* cbData = data.GetParameter();
        printf("Got gc callback for %s\n", cbData->key.c_str());
        cb_vec_t& vec = cbData->parent->m_callbacks[cbData->key];
        vec.erase(std::remove(vec.begin(), vec.end(), data.GetValue()), vec.end());
        delete cbData;
    }
    
    void NetworkTable_node::RemoveListener(const FunctionCallbackInfo<Value>& args) {
        std::lock_guard<std::mutex> _lock(pendingUpdatesLock);
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        if(!args[0]->IsString() || !args[1]->IsFunction()) {
            isolate->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(isolate, "Key name and callback must be provided")));
            return;
        }
        
        Local<String> v8str = args[0]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* key = *v8StrVal;
        
        Local<Function> func = Local<Function>::Cast(args[1]);
        
        std::string keyStr = key;
        cb_vec_t& vec = obj->m_callbacks[keyStr];
        vec.erase(std::remove(vec.begin(), vec.end(), func), vec.end());
    }

    void NetworkTable_node::Test(const FunctionCallbackInfo<Value>& args) {
        Isolate* isolate = args.GetIsolate();
        
        NetworkTable_node* obj = ObjectWrap::Unwrap<NetworkTable_node>(args.Holder());
        Local<String> v8str = args[0]->ToString();
        String::Utf8Value v8StrVal(v8str);
        char* key = *v8StrVal;
        
        obj->ValueChanged(NULL, key, nt::Value::MakeDouble(42), false);
        
        args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, obj->mp_name));
    }
    
    void NetworkTable_node::ValueChanged(ITable* source, llvm::StringRef key, std::shared_ptr<nt::Value> ntValue, bool isNew) {
        // since this is the NT thread, we need to queue the callback to run on the uv thread
        std::string keyStr = key.str();
        {
            // add the update to the queue of updates to run callbacks on
            std::lock_guard<std::mutex> _lock(pendingUpdatesLock);
            pendingUpdates.push({this, keyStr, ntValue});
        }
        // queue the callback handler on the uv thread
        uv_async_send(&asyncHandle);
    }
    
    void NetworkTable_node::RunCallbackAsync(uv_async_t* handle) {
        std::lock_guard<std::mutex> _lock(pendingUpdatesLock);
        // get all the v8 variables we need to call functions
        Isolate* isolate = Isolate::GetCurrent();
        if(isolate == NULL){
            printf("ntcore_node error: isolate == null!\n");
            return;
        }
        HandleScope scope(isolate);
        Local<Context> ctx = isolate->GetCurrentContext();
        Local<Object> global = ctx->Global();
        while(pendingUpdates.size() > 0){
            PendingUpdate& upd = pendingUpdates.front();
            Local<String> v8Key = String::NewFromUtf8(isolate, upd.key.c_str());
            // find all callbacks for that key
            for(cb_vec_t::iterator it = upd.parent->m_callbacks[upd.key].begin(); it != upd.parent->m_callbacks[upd.key].end(); ++it){
                cb_persist_t& cb = *it;
                // get a local Function object out of the heap storage
                Local<Function> cb_l = Local<Function>::New(isolate, cb);
                // set arguments and call
                Local<Value> args[2];
                args[0] = v8Key;
                if(upd.ntValue->IsBoolean()){
                    args[1] = Boolean::New(isolate, upd.ntValue->GetBoolean());
                } else if(upd.ntValue->IsDouble()){
                    args[1] = Number::New(isolate, upd.ntValue->GetDouble());
                } else if(upd.ntValue->IsString()){
                    args[1] = String::NewFromUtf8(isolate, upd.ntValue->GetString().str().c_str());
                } else if(upd.ntValue->IsBooleanArray() || upd.ntValue->IsDoubleArray() || upd.ntValue->IsStringArray()) {
                    Local<Array> v8Arr = Array::New(isolate);
                    if(upd.ntValue->IsBooleanArray()){
                        llvm::ArrayRef<int> ntArr = upd.ntValue->GetBooleanArray();
                        for(uint32_t i = 0; i < ntArr.size(); i++){
                            v8Arr->Set(isolate->GetCurrentContext(), i, Boolean::New(isolate, ntArr[i]));
                        }
                    }
                    if(upd.ntValue->IsDoubleArray()){
                        llvm::ArrayRef<double> ntArr = upd.ntValue->GetDoubleArray();
                        for(uint32_t i = 0; i < ntArr.size(); i++){
                            v8Arr->Set(isolate->GetCurrentContext(), i, Number::New(isolate, ntArr[i]));
                        }
                    }
                    if(upd.ntValue->IsStringArray()){
                        llvm::ArrayRef<std::string> ntArr = upd.ntValue->GetStringArray();
                        for(uint32_t i = 0; i < ntArr.size(); i++){
                            v8Arr->Set(isolate->GetCurrentContext(), i, String::NewFromUtf8(isolate, ntArr[i].c_str()));
                        }
                    }
                
                    args[1] = v8Arr;
                } else {
                    break;
                }
                cb_l->Call(ctx, global, 2, args);
            }
            pendingUpdates.pop();
        }
    }
}