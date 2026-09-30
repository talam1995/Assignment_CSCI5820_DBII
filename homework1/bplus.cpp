// bplus.cpp
// Build:  g++ -std=c++17 -O2 -o bplus bplus.cpp
// Usage:  ./bplus init <d> < commands.txt
//
// The program starts with an empty tree whose node capacity is set by d
// d is the order of the tree
// It runs every command in the input in order, and exits

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// Node

// Capacity for order d (every node except the root must be at least half full):
// - Leaves hold between d and 2d entries
// - Internal nodes hold d to 2d keys (so d+1 to 2d+1 children)
// - Root is the only node that can be less than half full
// Doubly link the leaves to make range queries easier

struct Node {
    bool isLeaf = true;
    vector<int> keys;
    vector<int> pointers;          // Data Pointers for Leaves
    vector<Node *> children;       // Child Nodes for Internal Nodes
    Node *next = nullptr;          // Next Leaf
    Node *prev = nullptr;          // Previous Leaf
};

// B+ Tree
// Internal node routing: branch to child i where keys[i-1] <= K < keys[i]
// (i.e., child index = count of keys <= K)

class BPlusTree {
public:
    explicit BPlusTree(int order) : d(order) {
        root = new Node();         // start with a single empty leaf root
    }

    ~BPlusTree() { freeSubtree(root); }

    BPlusTree(const BPlusTree &) = delete;
    BPlusTree &operator=(const BPlusTree &) = delete;

    // Returns the pointer for a given key, or -1 if not found
    // Note: use lookup() directly if -1 is a valid pointer value in the database
    int search(int key) const {
        int pointer;
        return lookup(key, pointer) ? pointer : -1;
    }

    // Same search, but reports "found" separately (from pointer value)
    bool lookup(int key, int &pointer) const {
        const Node *leaf = findLeaf(key);
        auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        if (it == leaf->keys.end() || *it != key) return false;
        pointer = leaf->pointers[it - leaf->keys.begin()];
        return true;
    }

    // Inserts (key, pointer)
    // Returns false if the key already exists
    bool insert(int key, int pointer) {
        bool duplicate = false;
        int sepKey;
        Node *newNode = nullptr;
        bool split = insertRec(root, key, pointer, duplicate, sepKey, newNode);
        if (duplicate) return false;

        if (split) {
            // The old root split, grow the tree by one level
            Node *newRoot = new Node();
            newRoot->isLeaf = false;
            newRoot->keys = {sepKey};
            newRoot->children = {root, newNode};
            root = newRoot;
            height++;
            numNodes++;
        }
        numKeys++;
        return true;
    }

    // Deletes key returns false if the key is not present
    // This is named remove because "delete" is a reserved word in C++
    bool remove(int key) {
        Result r = deleteRec(root, key);
        if (r == NOT_FOUND) return false;

        // If it has no keys left, delete it and point root to its only child
        if (!root->isLeaf && root->keys.empty()) {
            Node *oldRoot = root;
            root = root->children[0];
            delete oldRoot;
            height--;
            numNodes--;
        }
        numKeys--;
        return true;
    }

    // Returns all (key, pointer) pairs with k1 <= key <= k2 in key order
    // An empty vector plays the role of null
    vector<pair<int, int>> rangeSearch(int k1, int k2) const {
        vector<pair<int, int>> out;
        if (k1 > k2) return out;
        for (const Node *leaf = findLeaf(k1); leaf != nullptr; leaf = leaf->next) {
            for (size_t i = 0; i < leaf->keys.size(); i++) {
                if (leaf->keys[i] > k2) return out;
                if (leaf->keys[i] >= k1) out.push_back({leaf->keys[i], leaf->pointers[i]});
            }
        }
        return out;
    }

    // Prints the tree level by level: [] is one node, () is one entry
    void print() const {
        vector<const Node *> level = {root};
        int depth = 0;
        while (!level.empty()) {
            vector<const Node *> nextLevel;
            string line = "Level " + to_string(depth) + ":";
            for (const Node *n : level) {
                line += " [";
                for (size_t i = 0; i < n->keys.size(); i++) {
                    if (i > 0) line += " ";
                    if (n->isLeaf)
                        line += "(" + to_string(n->keys[i]) + ": " + to_string(n->pointers[i]) + ")";
                    else
                        line += "(" + to_string(n->keys[i]) + ")";
                }
                line += "]";
                if (!n->isLeaf)
                    nextLevel.insert(nextLevel.end(), n->children.begin(), n->children.end());
            }
            cout << line << "\n";
            level.swap(nextLevel);
            depth++;
        }
    }

    void printStatistics() const {
        cout << "Tree Height: " << height << "\n";
        cout << "Total Nodes: " << numNodes << "\n";
        cout << "Total Keys: " << numKeys << "\n";
    }

private:
    const int d;
    Node *root;
    int height = 1;         // Number of levels (a lone root leaf = 1)
    long long numNodes = 1;
    long long numKeys = 0;

    enum Result { NOT_FOUND, OK, UNDERFLOW };

    int maxKeys() const { return 2 * d; }
    int minKeys() const { return d; }     // Same bound for leaves and internal nodes

    static int childIndex(const Node *n, int key) {
        return static_cast<int>(upper_bound(n->keys.begin(), n->keys.end(), key) - n->keys.begin());
    }

    // Descends from the root to the leaf where key belongs
    const Node *findLeaf(int key) const {
        const Node *n = root;
        while (!n->isLeaf) n = n->children[childIndex(n, key)];
        return n;
    }

    static void freeSubtree(Node *n) {
        if (!n->isLeaf)
            for (Node *c : n->children) freeSubtree(c);
        delete n;
    }

    
    // Inserts into the subtree rooted at node
    // Returns true if node was split
    // newNode is the new right node
    // sepKey is the entry must add to the parent
    bool insertRec(Node *node, int key, int pointer, bool &duplicate,
                   int &sepKey, Node *&newNode) {
        if (node->isLeaf) {
            auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
            if (it != node->keys.end() && *it == key) {
                duplicate = true;
                return false;
            }
            size_t pos = it - node->keys.begin();
            node->keys.insert(node->keys.begin() + pos, key);
            node->pointers.insert(node->pointers.begin() + pos, pointer);
            if ((int)node->keys.size() <= maxKeys()) return false;
            splitLeaf(node, sepKey, newNode);
            return true;
        }

        int ci = childIndex(node, key);
        int childSep;
        Node *childNew = nullptr;
        if (!insertRec(node->children[ci], key, pointer, duplicate, childSep, childNew))
            return false;                         // usual case is no split below

        node->keys.insert(node->keys.begin() + ci, childSep);
        node->children.insert(node->children.begin() + ci + 1, childNew);
        if ((int)node->keys.size() <= maxKeys()) return false;
        splitInternal(node, sepKey, newNode);
        return true;
    }

    // Leaf split (2d+1 entries): the first d entries stay
    // the other d+1 move to a new leaf
    // the new leaf's smallest key is copied up
    void splitLeaf(Node *left, int &sepKey, Node *&newNode) {
        Node *right = new Node();
        right->keys.assign(left->keys.begin() + d, left->keys.end());
        right->pointers.assign(left->pointers.begin() + d, left->pointers.end());
        left->keys.resize(d);
        left->pointers.resize(d);

        right->prev = left;                   // splice into the leaf chain
        right->next = left->next;
        if (left->next) left->next->prev = right;
        left->next = right;

        numNodes++;
        sepKey = right->keys[0];
        newNode = right;
    }

    // Internal split (2d+1 keys, 2d+2 children)
    // The first d keys and d+1
    // The middle key is pushed up
    // the last d keys and d+1 children move to a new node
    void splitInternal(Node *left, int &sepKey, Node *&newNode) {
        sepKey = left->keys[d];
        Node *right = new Node();
        right->isLeaf = false;
        right->keys.assign(left->keys.begin() + d + 1, left->keys.end());
        right->children.assign(left->children.begin() + d + 1, left->children.end());
        left->keys.resize(d);
        left->children.resize(d + 1);

        numNodes++;
        newNode = right;
    }

    // Deletes key from the subtree rooted at node
    // NOT_FOUND means key is not in the tree, nothing changed
    // OK means done
    // UNDERFLOW means node is now below minimum occupancy    
    Result deleteRec(Node *node, int key) {
        if (node->isLeaf) {
            auto it = lower_bound(node->keys.begin(), node->keys.end(), key);
            if (it == node->keys.end() || *it != key) return NOT_FOUND;
            size_t pos = it - node->keys.begin();
            node->keys.erase(node->keys.begin() + pos);
            node->pointers.erase(node->pointers.begin() + pos);
            return (int)node->keys.size() < minKeys() ? UNDERFLOW : OK;
        }

        int ci = childIndex(node, key);
        Result r = deleteRec(node->children[ci], key);
        if (r != UNDERFLOW) return r;

        fixUnderflow(node, ci);
        return (int)node->keys.size() < minKeys() ? UNDERFLOW : OK;
    }

    // The child at index ci of parent is under-full
    // Look at one sibling (the left one if it exists, otherwise the right one)
    // If the sibling has entries to spare, redistribute evenly
    // otherwise merge the right node of the pair into the left one and drop the right node's entry from parent
    void fixUnderflow(Node *parent, int ci) {
        bool siblingIsLeft = ci > 0;
        int sepIdx = siblingIsLeft ? ci - 1 : ci;          // parent key between the pair
        Node *left = parent->children[sepIdx];
        Node *right = parent->children[sepIdx + 1];
        Node *sib = siblingIsLeft ? left : right;

        if ((int)sib->keys.size() > minKeys()) {
            if (left->isLeaf) {
                redistributeLeaves(left, right);
                parent->keys[sepIdx] = right->keys[0];     // new low key of right node
            } else {
                parent->keys[sepIdx] = redistributeInternal(left, right, parent->keys[sepIdx]);
            }
            return;
        }

        if (left->isLeaf) {
            mergeLeaves(left, right);
        } else {
            // Pull the splitting key down from the parent into the merged node
            left->keys.push_back(parent->keys[sepIdx]);
            left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
            left->children.insert(left->children.end(), right->children.begin(), right->children.end());
            delete right;
            numNodes--;
        }
        parent->keys.erase(parent->keys.begin() + sepIdx);
        parent->children.erase(parent->children.begin() + sepIdx + 1);
    }

    // Splits the entries of two adjacent leaves evenly
    static void redistributeLeaves(Node *left, Node *right) {
        vector<int> keys = left->keys, ptrs = left->pointers;
        keys.insert(keys.end(), right->keys.begin(), right->keys.end());
        ptrs.insert(ptrs.end(), right->pointers.begin(), right->pointers.end());
        size_t half = keys.size() / 2;
        left->keys.assign(keys.begin(), keys.begin() + half);
        left->pointers.assign(ptrs.begin(), ptrs.begin() + half);
        right->keys.assign(keys.begin() + half, keys.end());
        right->pointers.assign(ptrs.begin() + half, ptrs.end());
    }

    // Redistributes two adjacent internal nodes through their parent key
    
    static int redistributeInternal(Node *left, Node *right, int parentKey) {
        vector<int> keys = left->keys;
        vector<Node *> kids = left->children;
        keys.push_back(parentKey);
        keys.insert(keys.end(), right->keys.begin(), right->keys.end());
        kids.insert(kids.end(), right->children.begin(), right->children.end());
        size_t leftKids = kids.size() / 2;
        int newParentKey = keys[leftKids - 1];
        left->keys.assign(keys.begin(), keys.begin() + (leftKids - 1));
        left->children.assign(kids.begin(), kids.begin() + leftKids);
        right->keys.assign(keys.begin() + leftKids, keys.end());
        right->children.assign(kids.begin() + leftKids, kids.end());
        return newParentKey;
    }

    // Moves all entries of the right leaf into the left leaf
    // unlinks the right leaf from the leaf chain and frees it
    void mergeLeaves(Node *left, Node *right) {
        left->keys.insert(left->keys.end(), right->keys.begin(), right->keys.end());
        left->pointers.insert(left->pointers.end(), right->pointers.begin(), right->pointers.end());
        left->next = right->next;
        if (right->next) right->next->prev = left;
        delete right;
        numNodes--;
    }
};


// Command processing

static string upper(string s) {
    for (char &c : s) c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    return s;
}

// Parses a whole decimal integer in [lo, hi] and rejects anything else
static bool parseInt(const string &tok, long long lo, long long hi, long long &value) {
    try {
        size_t used = 0;
        long long v = stoll(tok, &used);
        if (used != tok.size() || v < lo || v > hi) return false;
        value = v;
        return true;
    } catch (...) {
        return false;
    }
}

// Reads one 32-bit integer argument from a command line
static bool readInt(istringstream &in, int &value) {
    string tok;
    long long v;
    if (!(in >> tok) || !parseInt(tok, INT32_MIN, INT32_MAX, v)) return false;
    value = static_cast<int>(v);
    return true;
}

static bool noMoreTokens(istringstream &in) {
    string extra;
    return !(in >> extra);
}

// Runs one command line
// Malformed lines are reported on stderr so that stdout only carries the output
static void runCommand(BPlusTree &tree, const string &line, int lineNo) {
    istringstream in(line);
    string cmd;
    if (!(in >> cmd)) return;                   // blank line
    cmd = upper(cmd);
    int a, b;

    if (cmd == "SEARCH") {
        if (readInt(in, a) && noMoreTokens(in)) {
            int ptr;
            if (tree.lookup(a, ptr)) cout << a << " found, point is " << ptr << "\n";
            else cout << a << " not found\n";
            return;
        }
    } else if (cmd == "INSERT") {
        if (readInt(in, a) && readInt(in, b) && noMoreTokens(in)) {
            if (tree.insert(a, b)) cout << "(" << a << ", " << b << ") inserted\n";
            else cout << "(" << a << ", " << b << ") not inserted. " << a << " found.\n";
            return;
        }
    } else if (cmd == "DELETE") {
        if (readInt(in, a) && noMoreTokens(in)) {
            if (tree.remove(a)) cout << a << " deleted.\n";
            else cout << a << " not found, not deleted.\n";
            return;
        }
    } else if (cmd == "RANGESEARCH") {
        if (readInt(in, a) && readInt(in, b) && noMoreTokens(in)) {
            vector<pair<int, int>> found = tree.rangeSearch(a, b);
            if (found.empty()) {
                cout << "no records in the range [" << a << ", " << b << "]\n";
            } else {
                cout << "found\n";
                for (auto &kv : found) cout << "(" << kv.first << ", " << kv.second << ")\n";
            }
            return;
        }
    } else if (cmd == "PRINT") {
        string rest;
        if (!(in >> rest)) {
            tree.print();
            return;
        }
        if (upper(rest) == "STATISTICS" && noMoreTokens(in)) {
            tree.printStatistics();
            return;
        }
    }
    cerr << "line " << lineNo << ": invalid command ignored: " << line << "\n";
}

int main(int argc, char *argv[]) {
    ios::sync_with_stdio(false);

    // Expected invocation: bplus init <d> < commands.txt
    const long long MAX_D = INT_MAX / 4;        // keeps 2d + 2 well inside int range
    long long d = 0;
    if (argc != 3 || upper(argv[1]) != "INIT" || !parseInt(argv[2], 1, MAX_D, d)) {
        cerr << "usage: bplus init <d> < commands.txt   (d is an integer >= 1)\n";
        return 1;
    }

    BPlusTree tree(static_cast<int>(d));
    string line;
    int lineNo = 0;
    while (getline(cin, line)) {
        lineNo++;
        if (!line.empty() && line.back() == '\r') line.pop_back();   // Windows line endings
        if (lineNo == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);  // UTF-8 BOM
        runCommand(tree, line, lineNo);
    }
    return 0;
}