#include "Util/include/Trie.hpp"
#include <queue>

// TrieNode
TrieNode::TrieNode() : isEndOfWord(false), indexType(IndexType::SINGLE) {
    index = -1;
}

TrieNode::~TrieNode() {
    if (indexType == IndexType::MULTIPLE) {
        delete indices;
    }
}

void TrieNode::addIndex(int newIndex) {
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

std::vector<int> TrieNode::getIndices() const {
    if (indexType == IndexType::SINGLE) {
        return (index == -1) ? std::vector<int>{} : std::vector<int>{index};
    }
    return *indices;
}


// Trie
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
    for (const char &c : word) {
        auto it = node->children.find(c);
        if (it == node->children.end()) {
            it = node->children.emplace(c, new TrieNode()).first;
        }
        node = it->second;
    }
    node->isEndOfWord = true;
    node->addIndex(index);
}

std::vector<int> Trie::searchPrefix(const std::string &prefix) const {
    if (prefix.empty()) {}
    TrieNode* node = root;
    for (const char &c : prefix) {
        auto it = node->children.find(c);
        if (it == node->children.end()) {
            return {};
        }
        node = it->second;
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
            stack.push_back(child.second);
        }
    }
    return results;
}