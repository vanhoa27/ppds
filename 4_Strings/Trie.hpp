#ifndef TRIE_H
#define TRIE_H
#include <string>
#include <unordered_map>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    bool isWordEnd;
    TrieNode();
};

class Trie {
public:
    Trie();
    ~Trie();

    void insertKey(const std::string& word) const;
    [[nodiscard]] bool search(const std::string& word) const;
    [[nodiscard]] bool startsWith(const std::string& word) const;
private:
    TrieNode* root;

    static void clear(const TrieNode* node);
};

#endif //TRIE_H
