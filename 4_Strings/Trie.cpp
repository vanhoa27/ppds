#include "Trie.hpp"

TrieNode::TrieNode() : isWordEnd(false) {}

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
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            node->children[c] = new TrieNode();
        }
        node = node->children[c];
    }
    node->isWordEnd = true;
}

bool Trie::search(const std::string& word) const {
    TrieNode* node = root;
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            return false;
        }
        node = node->children[c];
    }
    return node->isWordEnd;
}

bool Trie::startsWith(const std::string& word) const {
    TrieNode* node = root;
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            return false;
        }
        node = node->children[c];
    }
    return true;
}