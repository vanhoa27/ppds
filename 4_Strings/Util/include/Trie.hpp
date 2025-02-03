#ifndef TRIE_H
#define TRIE_H

#include <string>
#include <unordered_map>
#include <vector>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    std::vector<int> indices;
    bool isEndOfWord;

    TrieNode();
};

class Trie {
public:
    Trie();
    ~Trie();

    void insertKey(const std::string& word, int index);
    std::vector<int> searchPrefix(const std::string& prefix);

private:
    TrieNode* root;
    void clear(TrieNode* node);
};

#endif // TRIE_H