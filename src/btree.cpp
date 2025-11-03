#include "btree.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <cmath>
#include <unordered_map>

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

// Animation methods implementation

void BTree::setKeyPosition(Node* node, int keyIndex, Vector2 position) {
    if (nodeKeyPositions.find(node) == nodeKeyPositions.end()) {
        nodeKeyPositions[node] = std::vector<Vector2>(node->keys.size());
    }
    if (keyIndex >= 0 && keyIndex < (int)nodeKeyPositions[node].size()) {
        nodeKeyPositions[node][keyIndex] = position;
    }
}

Vector2 BTree::getKeyTargetPosition(int key) {
    // Find where this key should be in the tree
    if (!root) return {400.0f, 200.0f};
    
    Node* current = root;
    std::vector<Node*> path;
    path.push_back(current);
    
    while (!current->leaf) {
        int i = 0;
        while (i < (int)current->keys.size() && key > current->keys[i]) i++;
        if (i < (int)current->children.size()) {
            current = current->children[i];
            path.push_back(current);
        } else {
            break;
        }
    }
    
    // Return position from the nodeKeyPositions map if available
    if (nodeKeyPositions.find(current) != nodeKeyPositions.end() && 
        !nodeKeyPositions[current].empty()) {
        // Find the index where this key would be inserted
        int idx = 0;
        while (idx < (int)current->keys.size() && key > current->keys[idx]) idx++;
        
        if (idx < (int)nodeKeyPositions[current].size()) {
            return nodeKeyPositions[current][idx];
        } else if (idx > 0 && (idx - 1) < (int)nodeKeyPositions[current].size()) {
            // Position after the last key
            Vector2 lastPos = nodeKeyPositions[current][idx - 1];
            return {lastPos.x + 60.0f, lastPos.y};
        }
    }
    
    return {400.0f, 200.0f}; // Default fallback
}

void BTree::updateAnimation(float deltaTime) {
    // Update current animations
    for (auto& anim : currentAnimations) {
        anim.progress += deltaTime / anim.duration;
        if (anim.progress > 1.0f) {
            anim.progress = 1.0f;
        }
        
        // Execute the operation when animation completes
        if (anim.progress >= 1.0f && !anim.completed) {
            anim.completed = true;
            
            if (anim.type == AnimationType::KeyMoving) {
                // Actually insert the key now
                if (anim.operation == AnimationStep::InsertKey) {
                    insertInternal(anim.movingKey);
                }
            } else if (anim.type == AnimationType::NodeOperation) {
                // Execute deletion or other node operations
                if (anim.operationKey != 0 && anim.operation == AnimationStep::None) {
                    eraseInternal(anim.operationKey);
                }
            }
        }
        
        // Update target position if needed
        if (anim.needsRecalculation && anim.type == AnimationType::KeyMoving) {
            anim.endPos = getKeyTargetPosition(anim.movingKey);
            anim.needsRecalculation = false;
        }
    }
    
    // Remove completed animations
    currentAnimations.erase(
        std::remove_if(currentAnimations.begin(), currentAnimations.end(),
            [](const AnimationStep& a) { return a.progress >= 1.0f && a.completed; }),
        currentAnimations.end()
    );
    
    // Start next animation if current is empty
    if (currentAnimations.empty() && !animationQueue.empty()) {
        processNextAnimation();
    }
}

void BTree::addAnimationStep(const AnimationStep& step) {
    animationQueue.push(step);
}

void BTree::processNextAnimation() {
    if (animationQueue.empty()) return;
    
    AnimationStep step = animationQueue.front();
    animationQueue.pop();
    
    step.progress = 0.0f;
    step.completed = false;
    currentAnimations.push_back(step);
}

void BTree::insertAnimated(int k) {
    // Create animation for key moving to target position
    AnimationStep moveAnim;
    moveAnim.type = AnimationType::KeyMoving;
    moveAnim.duration = 1.0f;
    moveAnim.movingKey = k;
    moveAnim.startPos = {400.0f, 50.0f}; // Top center of screen
    moveAnim.endPos = getKeyTargetPosition(k);
    moveAnim.needsRecalculation = true; // Will update as tree changes
    moveAnim.operation = AnimationStep::InsertKey;
    moveAnim.operationKey = k;
    
    addAnimationStep(moveAnim);
}

void BTree::insertInternal(int k) {
    // This is the actual insertion that happens after animation
    if (!root) {
        root = new Node(t, true);
        root->keys.push_back(k);
        all_keys.push_back(k);
        return;
    }
    
    // Check if root needs splitting
    if ((int)root->keys.size() == 2*t - 1) {
        // First, queue an animation showing the violation (node is full)
        AnimationStep violationAnim;
        violationAnim.type = AnimationType::KeyHighlight;
        violationAnim.duration = 0.5f;
        violationAnim.highlightNode = root;
        violationAnim.highlightKeyIndex = -1; // Highlight entire node
        violationAnim.highlightColor = RED;
        violationAnim.completed = false;
        addAnimationStep(violationAnim);
        
        // Then queue split animation for root
        AnimationStep splitAnim;
        splitAnim.type = AnimationType::NodeSplitting;
        splitAnim.duration = 1.0f;
        splitAnim.operationNode = root;
        splitAnim.operation = AnimationStep::SplitNode;
        
        // Store the keys that will be in left and right nodes after split
        splitAnim.keysToAnimate = root->keys;
        splitAnim.operationKey = root->keys[t - 1]; // Middle key that goes up
        splitAnim.completed = false;
        addAnimationStep(splitAnim);
        
        // Actually do the split
        Node* newRoot = new Node(t, false);
        newRoot->children.push_back(root);
        newRoot->splitChild(0, root);
        int i = (newRoot->keys[0] < k) ? 1 : 0;
        
        // Check if the child we're inserting into will also need splitting
        if ((int)newRoot->children[i]->keys.size() == 2*t - 1) {
            // Queue another split animation
            AnimationStep childSplitAnim;
            childSplitAnim.type = AnimationType::NodeSplitting;
            childSplitAnim.duration = 1.0f;
            childSplitAnim.operationNode = newRoot->children[i];
            childSplitAnim.operation = AnimationStep::SplitNode;
            childSplitAnim.keysToAnimate = newRoot->children[i]->keys;
            childSplitAnim.operationKey = newRoot->children[i]->keys[t - 1];
            childSplitAnim.completed = false;
            addAnimationStep(childSplitAnim);
        }
        
        newRoot->children[i]->insertNonFull(k);
        root = newRoot;
    } else {
        // Check if insertion will cause any splits down the path
        insertNonFullWithAnimation(root, k);
    }
    
    all_keys.push_back(k);
}

void BTree::insertNonFullWithAnimation(Node* node, int k) {
    // This method inserts and queues animations for any splits that occur
    int i = (int)node->keys.size() - 1;
    
    if (node->leaf) {
        node->keys.push_back(0);
        while (i >= 0 && node->keys[i] > k) {
            node->keys[i + 1] = node->keys[i];
            --i;
        }
        node->keys[i + 1] = k;
        
        // Check if node is now overfull (violation)
        if ((int)node->keys.size() >= 2 * t) {
            AnimationStep violationAnim;
            violationAnim.type = AnimationType::KeyHighlight;
            violationAnim.duration = 0.3f;
            violationAnim.highlightNode = node;
            violationAnim.highlightKeyIndex = -1;
            violationAnim.highlightColor = ORANGE;
            violationAnim.completed = false;
            addAnimationStep(violationAnim);
        }
    } else {
        while (i >= 0 && node->keys[i] > k) --i;
        ++i;
        
        if ((int)node->children[i]->keys.size() == 2 * t - 1) {
            // Queue violation animation
            AnimationStep violationAnim;
            violationAnim.type = AnimationType::KeyHighlight;
            violationAnim.duration = 0.4f;
            violationAnim.highlightNode = node->children[i];
            violationAnim.highlightKeyIndex = -1;
            violationAnim.highlightColor = ORANGE;
            violationAnim.completed = false;
            addAnimationStep(violationAnim);
            
            // Queue split animation
            AnimationStep splitAnim;
            splitAnim.type = AnimationType::NodeSplitting;
            splitAnim.duration = 1.0f;
            splitAnim.operationNode = node->children[i];
            splitAnim.operation = AnimationStep::SplitNode;
            splitAnim.keysToAnimate = node->children[i]->keys;
            splitAnim.operationKey = node->children[i]->keys[t - 1];
            splitAnim.completed = false;
            addAnimationStep(splitAnim);
            
            node->splitChild(i, node->children[i]);
            if (node->keys[i] < k) ++i;
        }
        insertNonFullWithAnimation(node->children[i], k);
    }
}

void BTree::eraseAnimated(int k) {
    // Check if key exists
    Node* node = root ? root->search(k) : nullptr;
    
    if (!node) return; // Key not found
    
    int idx = -1;
    for (int i = 0; i < (int)node->keys.size(); ++i) {
        if (node->keys[i] == k) {
            idx = i;
            break;
        }
    }
    
    if (idx < 0) return; // Key not found in node
    
    // Step 1: Highlight the key being deleted (red flash)
    AnimationStep highlightAnim;
    highlightAnim.type = AnimationType::KeyHighlight;
    highlightAnim.duration = 0.5f;
    highlightAnim.highlightNode = node;
    highlightAnim.highlightKeyIndex = idx;
    highlightAnim.highlightColor = RED;
    highlightAnim.completed = false;
    addAnimationStep(highlightAnim);
    
    // Step 2: Move key out animation (key flying away and fading)
    AnimationStep moveOutAnim;
    moveOutAnim.type = AnimationType::KeyMoving;
    moveOutAnim.duration = 0.8f;
    moveOutAnim.movingKey = k;
    moveOutAnim.operation = AnimationStep::DeleteKey; // Mark as deletion
    moveOutAnim.completed = false;
    moveOutAnim.targetNode = node;
    moveOutAnim.targetIndex = idx;
    // Start and end positions will be set in main.cpp based on current node position
    moveOutAnim.needsRecalculation = true;
    addAnimationStep(moveOutAnim);
    
    // Step 3: Actually perform the deletion after animation
    AnimationStep deleteAnim;
    deleteAnim.type = AnimationType::NodeOperation;
    deleteAnim.duration = 0.1f;
    deleteAnim.operation = AnimationStep::None;
    deleteAnim.operationKey = k;
    deleteAnim.completed = false;
    addAnimationStep(deleteAnim);
}

void BTree::eraseInternal(int k) {
    if (!root) return;
    
    // Check if node might become too small after deletion
    Node* node = root->search(k);
    if (node) {
        erase(k);
        
        // Remove from all_keys
        auto it = std::find(all_keys.begin(), all_keys.end(), k);
        if (it != all_keys.end()) {
            all_keys.erase(it);
        }
        
        // Check if any nodes need rebalancing after deletion
        // This is where merge animations would be added
        if (root && root->keys.empty() && !root->leaf && !root->children.empty()) {
            // Root became empty, promote its only child
            AnimationStep promoteAnim;
            promoteAnim.type = AnimationType::NodeOperation;
            promoteAnim.duration = 0.8f;
            promoteAnim.operationNode = root->children[0];
            promoteAnim.operation = AnimationStep::BalanceTree;
            promoteAnim.completed = false;
            addAnimationStep(promoteAnim);
            
            Node* oldRoot = root;
            root = root->children[0];
            oldRoot->children.clear();
            delete oldRoot;
        }
    }
}
