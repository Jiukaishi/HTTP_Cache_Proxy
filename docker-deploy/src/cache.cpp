#include "cache.h"

bool Cache::exist(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  bool res = cache_.find(key) != cache_.end();
  return res;
}
void Cache::put(const std::string &key, HttpResponse *value) {
  std::lock_guard<std::mutex> lock(mutex_);
  // not in cache
  if (cache_.find(key) == cache_.end()) {
    Node *node = new Node(key, value);
    cache_[key] = node;
    addToHead(node);
    if (cache_.size() > capacity) {
      Node *rm = removeTail();
      cache_.erase(rm->key);
      
      delete rm->value;
      
      delete rm;
    }
  } else {
    HttpResponse *oldresp = cache_[key]->value;
    cache_[key]->value = value;
    
    delete oldresp;
    
    moveToHead(cache_[key]);
  }
}
void Cache::erase(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  // not in cache
  if (cache_.find(key) != cache_.end()) {
    HttpResponse *oldresp = cache_[key]->value;
    Node *old_node = cache_[key];
    old_node->prev->next = old_node->next;
    old_node->next->prev = old_node->prev;

    delete oldresp;
    delete old_node;
    
    cache_.erase(key);
  }
}
HttpResponse *Cache::find(const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    moveToHead(cache_[key]);
    return cache_[key]->value;
  } else {
    return NULL;
  }
}