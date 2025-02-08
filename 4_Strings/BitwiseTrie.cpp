#include "Util/include/BitwiseTrie.hpp"

// TrieNode
TrieNode::~TrieNode() {
    if (indexType == IndexType::MULTIPLE) {
        delete indices;
    }
}

inline void TrieNode::addIndex(int newIndex) {
    if (indexType == IndexType::SINGLE) {
        if (index == -1) {
            index = newIndex;
        } else {
            indices = new std::vector<int>{index, newIndex};
            indexType = IndexType::MULTIPLE;
        }
    } else {
        indices->push_back(newIndex);
    }
}

inline std::vector<int> TrieNode::getIndices() const {
    if (indexType == IndexType::SINGLE) {
        return (index == -1) ? std::vector<int>{} : std::vector<int>{index};
    }
    return *indices;
}

// Trie
Trie::Trie() { root = new TrieNode(); }

void Trie::clear(TrieNode* node) {
    if (!node) return;
    for (auto& child : node->children) {
        if (child) clear(child);
    }
    delete node;
}

Trie::~Trie() {
    clear(root);
}

inline int Trie::charToBit(char c) {
    if (c >= '0' && c <= '9') return c - '0';      // 0 - 9
    if (c >= 'a' && c <= 'z') return c - 'a' + 10; // 10 - 35
    return 36;
}

// void Trie::collectIndices(TrieNode* node, std::vector<int>& indices) {
//     if (!node) return;
//     if (node->isEndOfWord) {
//         // const std::vector<int>& indices = node->getIndices();
//         // results.insert(results.end(), indices.begin(), indices.end());
//         std::vector<int> res = node->getIndices();
//         indices.insert(indices.end(), res.begin(), res.end());
//     }
//
//     for (TrieNode* child : node->children) {
//         collectIndices(child, indices);
//     }
// }

void Trie::insertKey(const std::string &word, int index) {
    TrieNode *node = root;
    for (const char &c: word) {
        int bit = charToBit(c);

        if (!(node->bitmap & (1ULL << bit))) {
            node->bitmap |= (1ULL << bit);
            node->children.push_back(new TrieNode());
        }

        int childIndex = __builtin_popcountll(node->bitmap & ((1ULL << bit) - 1));
        node = node->children[childIndex];
    }
    node->isEndOfWord = true;
    node->addIndex(index);
}

std::vector<int> Trie::searchPrefix(const std::string &prefix) {
    if (prefix.empty()) return {};

    TrieNode* node = root;
    for (const char &c: prefix) {
        int bit = charToBit(c);

        if (!(node->bitmap & (1ULL << bit))) return {};

        int childIndex = __builtin_popcountll(node->bitmap & ((1ULL << bit) - 1));
        node = node->children[childIndex];
    }

    std::vector<int> results;
    std::vector<TrieNode*> stack = {node};

    while (!stack.empty()) {
        TrieNode* current = stack.back();
        stack.pop_back();

        if (current->isEndOfWord) {
            const std::vector<int>& indices = current->getIndices();
            results.insert(results.end(), indices.begin(), indices.end());
        }

        for (auto& child : current->children) {
            if (child) stack.push_back(child);
        }
    }
    return results;
    // return indices;
}