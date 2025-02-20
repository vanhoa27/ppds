#include "Util/include/Trie.hpp"

// TrieNode
TrieNode::TrieNode() : isEndOfWord(false), indexType(IndexType::SINGLE) {
    children.fill(nullptr);
    index = -1;
}

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

inline int Trie::mapCharToIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';      // 0 - 9
    if (c >= 'a' && c <= 'z') return c - 'a' + 10; // 10 - 35
    return 36;
}

void Trie::insertKey(const std::string& word, int index) {
    TrieNode* node = root;
    for (const char& c : word) {
        int ch = mapCharToIndex(c);
        if (!node->children[ch]) {
            node->children[ch] = new TrieNode();
        }
        node = node->children[ch];
    }
    node->isEndOfWord = true;
    node->addIndex(index);
}

std::vector<int> Trie::searchPrefix(const std::string &prefix) const {
    if (prefix.empty()) {
        return {};
    }

    TrieNode* node = root;
    for (const char &c : prefix) {
        int ch = mapCharToIndex(c);
        if (!node->children[ch]) {
            return {};
        }
        node = node->children[ch];
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
}
