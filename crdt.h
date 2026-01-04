// crdt.h - Header file for OR-Set CRDT implementation
#ifndef CRDT_H
#define CRDT_H

#include <bits/stdc++.h>

using namespace std;

struct Tag {
    string replica_id;
    uint64_t counter;

    bool operator<(const Tag& other) const {
        return tie(replica_id, counter) < tie(other.replica_id, other.counter);
    }
    bool operator==(const Tag& other) const {
        return replica_id == other.replica_id && counter == other.counter;
    }
};

class ORSet {
  private:
    string replica_id;
    uint64_t local_counter;
    set<pair<string, Tag>> internal_set;
    unordered_set<string> element_cache; // cache for O(1) contains check

  public:
    ORSet(const string& id) : replica_id(id), local_counter(0) {}

    void add(const string& element) {
        local_counter++;
        Tag tag{replica_id, local_counter};
        internal_set.insert({element, tag});
        element_cache.insert(element); // update the cache
        // Broadcast "add element with tag" to other replicas
    }

    void remove(const string& element) {
        set<Tag> tags_to_remove;
        for (const auto &pair : internal_set) {
            if(pair.first == element) {
                tags_to_remove.insert(pair.second);
            }
        }

        for (const auto &tag : tags_to_remove) {
            internal_set.erase({element, tag});
        }

        // check if element still exists after removal
        bool still_exists = false;
        for (const auto &pair : internal_set) {
            if(pair.first == element) {
                still_exists = true;
                break;
            }
        }
        if (!still_exists) {
            element_cache.erase(element); // update cache
        }
        // Broadcast "remove element with tags_to_remove" to other replicas
    }

    bool contains(const string& element) const {
        return element_cache.count(element) > 0; // O(1) lookup
    }

    set<string> elements() const {
        return set<string>(element_cache.begin(), element_cache.end());
    }

    void merge(const ORSet& other) {
        internal_set.insert(other.internal_set.begin(), other.internal_set.end());
        element_cache.insert(other.element_cache.begin(), other.element_cache.end()); // update the cache
    }

    // Additional methods for benchmarking
    size_t size() const { return element_cache.size(); }
    size_t internal_size() const { return internal_set.size(); }
    uint64_t get_counter() const { return local_counter; }
};

#endif