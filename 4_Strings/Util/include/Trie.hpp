#ifndef TRIE_H
#define TRIE_H

#include <array>
#include <string>
#include <vector>

class TrieNode {
public:
    static constexpr int CHAR_SET_SIZE = 37;
    std::array<TrieNode*, CHAR_SET_SIZE> children{};
    bool isEndOfWord;
    enum class IndexType { SINGLE, MULTIPLE } indexType;

    union {
        int index;
        std::vector<int>* indices;
    };

    TrieNode();
    ~TrieNode();
    void addIndex(int newIndex);
    [[nodiscard]] std::vector<int> getIndices() const;
};

class Trie {
public:
    Trie();
    ~Trie();

    void insertKey(const std::string& word, int index);
    [[nodiscard]] std::vector<int> searchPrefix(const std::string& prefix) const;
private:
    TrieNode* root;
    void clear(TrieNode* node);
    [[nodiscard]] static char mapCharToIndex(char c);
};

#endif // TRIE_H