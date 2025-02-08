#ifndef BITWISE_TRIE_H
#define BITWISE_TRIE_H

#include <cstdint>
#include <string>
#include <vector>

class TrieNode {
public:
    uint64_t bitmap:37 = 0;
    std::vector<TrieNode*> children;
    bool isEndOfWord = false;
    enum class IndexType { SINGLE, MULTIPLE } indexType;

    union {
        int index = -1;
        std::vector<int>* indices;
    };

    TrieNode() : indexType(IndexType::SINGLE){
    };

    ~TrieNode();
    inline void addIndex(int newIndex);
    [[nodiscard]] inline std::vector<int> getIndices() const;
};

class Trie {
public:
    Trie();
    ~Trie();

    void insertKey(const std::string& word, int index);
    [[nodiscard]] std::vector<int> searchPrefix(const std::string& prefix);
private:
    TrieNode* root;
    void clear(TrieNode* node);
    // void collectIndices(TrieNode* node, std::vector<int>& indices);
    [[nodiscard]] inline int charToBit(char c);
};

#endif // BITWISE_TRIE_H
