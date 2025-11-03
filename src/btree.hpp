#ifndef BTREE_HPP
#define BTREE_HPP

#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <unordered_map>
#include <raylib.h>

class BTree {
public:
    struct Node {
        bool leaf;
        int t; 
        std::vector<int> keys;
        std::vector<Node*> children;

        Node(int _t, bool _leaf);
        ~Node();

        void traverse(const std::function<void(Node*, int, int)>& cb, int depth = 0);
        Node* search(int k);

        
        void insertNonFull(int k);
        void splitChild(int idx, Node* y);
    };

    // Animation structures
    enum class AnimationType {
        None,
        KeyMoving,        // Key moving to target position
        NodeSplitting,    // Node being split
        NodeMerging,      // Nodes being merged
        KeyHighlight,     // Highlight a key/node during operation
        NodeOperation     // Generic node operation (insert actual key, split, merge)
    };

    struct AnimationStep {
        AnimationType type;
        float progress;  // 0.0 to 1.0
        float duration;  // in seconds
        bool completed;  // Whether the actual operation has been executed
        
        // For KeyMoving
        int movingKey;
        Vector2 startPos;
        Vector2 endPos;
        Node* targetNode;
        int targetIndex;
        bool needsRecalculation; // Recalculate end position based on tree state
        
        // For NodeSplitting/NodeOperation
        Node* operationNode;
        std::vector<int> keysToAnimate;
        std::vector<Vector2> keyStartPositions;
        std::vector<Vector2> keyEndPositions;
        
        // Operation details
        enum Operation {
            None,
            InsertKey,
            DeleteKey,
            SplitNode,
            MergeNode,
            BalanceTree
        } operation;
        
        int operationKey; // The key involved in the operation
        
        // For highlighting
        Node* highlightNode;
        int highlightKeyIndex;
        Color highlightColor;
    };

    BTree(int t = 2);
    ~BTree();

    void insert(int k);
    bool contains(int k) const;
    void erase(int k); 

    void clear();
    
    void clearAll();
    
    int getLastInsertedKey() const { return all_keys.empty() ? -1 : all_keys.back(); }
    bool hasKeys() const { return !all_keys.empty(); }

    
    void traverse(const std::function<void(Node*, int, int)>& cb);

    Node* getRoot() const { return root; }
    
    // Animation methods
    void updateAnimation(float deltaTime);
    bool isAnimating() const { return !animationQueue.empty() || !currentAnimations.empty(); }
    const std::vector<AnimationStep>& getCurrentAnimations() const { return currentAnimations; }
    bool hasAnimationJustCompleted() const { return animationJustCompleted; }
    void clearAnimationCompletedFlag() { animationJustCompleted = false; }
    
    // Method to provide layout information from main.cpp
    void setKeyPosition(Node* node, int keyIndex, Vector2 position);
    Vector2 getKeyTargetPosition(int key);
    
    // Node position tracking
    std::unordered_map<Node*, std::vector<Vector2>> nodeKeyPositions;
    
    // Animated insert/delete
    void insertAnimated(int k);
    void eraseAnimated(int k);

private:
    Node* root;
    int t;
    std::vector<int> all_keys;
    
    // Animation state
    std::queue<AnimationStep> animationQueue;
    std::vector<AnimationStep> currentAnimations;
    bool animationJustCompleted = false;
    
    void addAnimationStep(const AnimationStep& step);
    void processNextAnimation();
    
    // Internal methods for actual operations (called after animation)
    void insertInternal(int k);
    void insertNonFullWithAnimation(Node* node, int k);
    void eraseInternal(int k);
    Node* findInsertionNode(int k, std::vector<Node*>& path);
};

#endif 
