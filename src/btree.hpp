#ifndef BTREE_HPP
#define BTREE_HPP

#include <vector>
#include <memory>
#include <functional>




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

    BTree(int t = 2);
    ~BTree();

    void insert(int k);
    bool contains(int k) const;
    void erase(int k); 

    void clear();
    
    void clearAll();

    
    void traverse(const std::function<void(Node*, int, int)>& cb);

    Node* getRoot() const { return root; }

private:
    Node* root;
    int t;
    std::vector<int> all_keys; 
};

#endif 
