#include "btree.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>


BTree::Node::Node(int _t, bool _leaf) : leaf(_leaf), t(_t) {}

BTree::Node::~Node() {
    for (auto c : children) delete c;
}

void BTree::Node::traverse(const std::function<void(Node*, int, int)>& cb, int depth) {
    int i;
    for (i = 0; i < (int)keys.size(); ++i) {
        if (!leaf && i < (int)children.size()) children[i]->traverse(cb, depth + 1);
        cb(this, depth, i);
    }
    if (!leaf && children.size()) children.back()->traverse(cb, depth + 1);
}

BTree::Node* BTree::Node::search(int k) {
    int i = 0;
    while (i < (int)keys.size() && k > keys[i]) i++;
    if (i < (int)keys.size() && keys[i] == k) return this;
    if (leaf) return nullptr;
    return children[i]->search(k);
}

void BTree::Node::insertNonFull(int k) {
    int i = (int)keys.size() - 1;
    if (leaf) {
        keys.push_back(0);
        while (i >= 0 && keys[i] > k) {
            keys[i + 1] = keys[i];
            --i;
        }
        keys[i + 1] = k;
    } else {
        while (i >= 0 && keys[i] > k) --i;
        ++i;
        if ((int)children[i]->keys.size() == 2 * t - 1) {
            splitChild(i, children[i]);
            if (keys[i] < k) ++i;
        }
        children[i]->insertNonFull(k);
    }
}

void BTree::Node::splitChild(int idx, Node* y) {
    
    Node* z = new Node(y->t, y->leaf);
    
    int midKey = y->keys[t - 1];
    
    z->keys.assign(y->keys.begin() + t, y->keys.end());
    
    if (!y->leaf) {
        z->children.assign(y->children.begin() + t, y->children.end());
    }
    
    y->keys.resize(t - 1);
    if (!y->leaf) y->children.resize(t);

    
    children.insert(children.begin() + idx + 1, z);
    
    keys.insert(keys.begin() + idx, midKey);
}


BTree::BTree(int _t) : root(nullptr), t(_t) {}

BTree::~BTree() { clear(); }

void BTree::insert(int k) {
    if (!root) {
        root = new Node(t, true);
        root->keys.push_back(k);
        all_keys.push_back(k);
        return;
    }
    if (root->keys.size() == (size_t)(2 * t - 1)) {
        Node* s = new Node(t, false);
        s->children.push_back(root);
        s->splitChild(0, root);
        int i = 0;
        if (s->keys[0] < k) i++;
        s->children[i]->insertNonFull(k);
        root = s;
    } else {
        root->insertNonFull(k);
    }
    all_keys.push_back(k);
}

bool BTree::contains(int k) const {
    if (!root) return false;
    return root->search(k) != nullptr;
}

void BTree::erase(int k) {
    
    
    auto it = std::find(all_keys.begin(), all_keys.end(), k);
    if (it == all_keys.end()) return;
    
    std::vector<int> remaining;
    remaining.reserve(all_keys.size() - 1);
    bool skipped = false;
    for (size_t i = 0; i < all_keys.size(); ++i) {
        if (!skipped && all_keys[i] == k) { skipped = true; continue; }
        remaining.push_back(all_keys[i]);
    }
    
    clear();
    all_keys.clear();
    for (int key : remaining) insert(key);
}

void BTree::clear() {
    if (root) { delete root; root = nullptr; }
    
}

void BTree::clearAll() {
    clear();
    all_keys.clear();
}

void BTree::traverse(const std::function<void(Node*, int, int)>& cb) {
    if (root) root->traverse(cb, 0);
}
