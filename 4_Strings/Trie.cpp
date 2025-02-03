#include "Util/include/Trie.hpp"

TrieNode::TrieNode() : isEndOfWord(false) {}

Trie::Trie() { root = new TrieNode(); }

void Trie::clear(TrieNode* node) {
    for (auto& child : node->children) {
        clear(child.second);
    }
    delete node;
}

Trie::~Trie() {
    clear(root);
}

void Trie::insertKey(const std::string& word, int index) {
    TrieNode* node = root;
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            node->children[c] = new TrieNode();
        }
        node = node->children[c];
    }
    node->isEndOfWord = true;
    node->indices.push_back(index);
}

std::vector<int> Trie::searchPrefix(const std::string& prefix) {
    TrieNode* node = root;
    for (const char& c : prefix) {
        if (!node->children.contains(c)) {
            return {};
        }
        node = node->children[c];
    }

    std::vector<int> results;
    std::vector<TrieNode*> stack = {node};

    while (!stack.empty()) {
        TrieNode* current = stack.back();
        stack.pop_back();

        if (current->isEndOfWord) {
            results.insert(results.end(), current->indices.begin(), current->indices.end());
        }

        for (auto& child : current->children) {
            stack.push_back(child.second);
        }
    }

    return results;
}
