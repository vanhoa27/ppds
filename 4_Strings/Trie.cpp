#include "Util/include/Trie.hpp"

TrieNode::TrieNode() = default;

Trie::Trie() { root = new TrieNode(); }

void Trie::clear(const TrieNode* node) {
    for (auto& child : node->children) {
        clear(child.second);
    }
    delete node;
}

Trie::~Trie() {
    clear(root);
}

void Trie::insertKey(const std::string& word) const {
    TrieNode* node = root;
    int counter = 0;
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            node->children[c] = new TrieNode();
        }
        node = node->children[c];
        ++counter;
    }
    node->indices.push_back(counter);
}

std::vector<int> Trie::search(const std::string& word) {
    TrieNode* node = root;
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            return {};
        }
        node = node->children[c];
    }
    return node->indices;
}