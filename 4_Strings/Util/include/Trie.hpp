#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <unordered_map>
#include <vector>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    bool isEndOfWord;
    enum class IndexType { SINGLE, MULTIPLE } indexType;

    union {
        int index;
        std::vector<int>* indices;
    };

    TrieNode();
    ~TrieNode();
    void addIndex(int newIndex);
    std::vector<int> getIndices() const;
};

class Trie {
public:
    Trie();
    ~Trie();

    void insertKey(const std::string& word, int index);
    std::vector<int> searchPrefix(const std::string& prefix) const;

private:
    TrieNode* root;
    void clear(TrieNode* node);
};

#endif // TRIE_H