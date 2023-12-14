#include <unordered_map>
#include <mutex>
#include <string>
#include <thread>
#include "HttpResponse.h"
class Node {
    public:
    std::string key;
    HttpResponse* value;
    Node* prev;
    Node* next;
    Node(): key("dummy"), value(nullptr), prev(nullptr), next(nullptr) {}
    Node(std::string _key, HttpResponse* _value): key(_key), value(_value), prev(nullptr), next(nullptr) {}
};

class Cache {
    std::unordered_map<std::string, Node *> cache_;
    std::mutex mutex_;
    int size;
    int capacity;
    Node* head;
    Node* tail;
 public:
    Cache() 
    {
        size=0;
        capacity=100;
        head = new Node();
        tail = new Node();
        head->next = tail;
        tail->prev = head;
    }
    ~Cache() {
        for(auto ite=cache_.begin(); ite!=cache_.end(); ite++){
            delete ite->second; // delete node
        }
    }
    bool exist(const std::string& key);
    void put(const std::string& key, HttpResponse * value);
    void erase(const std::string& key);
    HttpResponse * find(const std::string& key);
    void addToHead(Node* node) {
        node->prev = head;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
    }
    
    void removeNode(Node* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void moveToHead(Node* node) {
        removeNode(node);
        addToHead(node);
    }

    Node* removeTail() {
       Node* node = tail->prev;
        removeNode(node);
        return node;
    }

};
