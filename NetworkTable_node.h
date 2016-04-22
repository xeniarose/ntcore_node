#ifndef NETWORKTABLE_NODE_H
#define NETWORKTABLE_NODE_H

#include <node.h>
#include <node_object_wrap.h>
#include <ntcore.h>
#include <networktables/NetworkTable.h>
#include <tables/ITableListener.h>
#include <string>
#include <unordered_map>
#include <uv.h>
#include <mutex>
#include <queue>

namespace ntcore_node {

typedef v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> cb_persist_t;
typedef std::vector<cb_persist_t> cb_vec_t;
typedef std::unordered_map<std::string, cb_vec_t> cb_map_t;
class NetworkTable_node;
struct PendingUpdate {
    NetworkTable_node* parent;
    std::string key;
    std::shared_ptr<nt::Value> ntValue;
};
struct CbWeakCallbackData {
    NetworkTable_node* parent;
    std::string key;
};

class NetworkTable_node : public node::ObjectWrap, public ITableListener {
public:
    static v8::Isolate* p_defaultIsolate;
    static std::vector<NetworkTable_node*> networkTables;
    
    static void Init(v8::Isolate* isolate);
    static void NewInstance(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    void ValueChanged(ITable* source, llvm::StringRef key, std::shared_ptr<nt::Value> value, bool isNew);
    
private:
    char* mp_name;
    std::shared_ptr<NetworkTable> mp_table;
    cb_map_t m_callbacks;
    static std::queue<PendingUpdate> pendingUpdates;
    static std::mutex pendingUpdatesLock;
    static uv_async_t asyncHandle;
    
    explicit NetworkTable_node(char* name);
    ~NetworkTable_node();
    
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void GetName(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Get(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Put(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Remove(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void AddListener(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void RemoveListener(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Test(const v8::FunctionCallbackInfo<v8::Value>& args);
    
    static void CbWeakCallback(const v8::WeakCallbackData<v8::Function, CbWeakCallbackData>& data);
    static void RunCallbackAsync(uv_async_t* handle);
    
    static v8::Persistent<v8::Function> constructor;
};

}

#endif