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

void Trie::insertKey(const std::string& word) {
    TrieNode* node = root;
    for (const char& c : word) {
        if (!node->children.contains(c)) {
            node->children[c] = new TrieNode();
        }
        node = node->children[c];
    }
    node->isEndOfWord = true;
}

std::vector<std::string> Trie::getPrefixes(const std::string& word) {
    TrieNode* node = root;
    std::vector<std::string> result;
    std::string prefix;

    for (const char& c : word) {
        if (!node->children.contains(c)) {
            break;
        }
        node = node->children[c];
        prefix += c;

        if (node->isEndOfWord) {  // If a valid word ends here, store the prefix
            result.push_back(prefix);
        }
    }

    return result;
}
