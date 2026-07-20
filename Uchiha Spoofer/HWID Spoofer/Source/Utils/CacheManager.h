#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>
#include <list>

namespace UchihaSpoofer::Utils {

// LRU Cache template cho caching Registry/System values
template<typename Key, typename Value>
class LRUCache {
private:
    struct Node {
        Key key;
        Value value;
        Node* prev;
        Node* next;

        Node(const Key& k, const Value& v)
            : key(k), value(v), prev(nullptr), next(nullptr) {}
    };

    size_t capacity_;
    std::unordered_map<Key, Node*> cache_;
    Node* head_; // Most recently used
    Node* tail_; // Least recently used
    mutable std::mutex mutex_;

    void moveToHead(Node* node) {
        if (node == head_) return;

        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (node == tail_) tail_ = node->prev;

        node->prev = nullptr;
        node->next = head_;
        if (head_) head_->prev = node;
        head_ = node;
        if (!tail_) tail_ = head_;
    }

    void removeTail() {
        if (!tail_) return;
        cache_.erase(tail_->key);
        Node* temp = tail_;
        tail_ = tail_->prev;
        if (tail_) tail_->next = nullptr;
        else head_ = nullptr;
        delete temp;
    }

public:
    explicit LRUCache(size_t capacity) : capacity_(capacity), head_(nullptr), tail_(nullptr) {}

    ~LRUCache() {
        while (head_) {
            Node* temp = head_;
            head_ = head_->next;
            delete temp;
        }
    }

    void Put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second->value = value;
            moveToHead(it->second);
            return;
        }

        Node* newNode = new Node(key, value);
        cache_[key] = newNode;
        moveToHead(newNode);

        if (cache_.size() > capacity_) {
            removeTail();
        }
    }

    bool Get(const Key& key, Value& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache_.find(key);
        if (it == cache_.end()) return false;

        moveToHead(it->second);
        value = it->second->value;
        return true;
    }

    bool Contains(const Key& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.find(key) != cache_.end();
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (head_) {
            Node* temp = head_;
            head_ = head_->next;
            delete temp;
        }
        cache_.clear();
        tail_ = nullptr;
    }

    size_t Size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }
};

// Registry value cache (với time-to-live)
class RegistryCache {
public:
    static RegistryCache& GetInstance() {
        static RegistryCache instance;
        return instance;
    }

    // Cache Registry reads
    bool GetCached(const std::string& path, const std::string& key, std::string& value);
    void SetCached(const std::string& path, const std::string& key, const std::string& value);
    void InvalidateCache();

private:
    RegistryCache() : cache_(100) {} // 100 item capacity
    RegistryCache(const RegistryCache&) = delete;
    RegistryCache& operator=(const RegistryCache&) = delete;

    LRUCache<std::string, std::string> cache_;

    std::string MakeCacheKey(const std::string& path, const std::string& key) const {
        return path + ":" + key;
    }
};

} // namespace UchihaSpoofer::Utils
