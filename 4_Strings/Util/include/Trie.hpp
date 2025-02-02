#ifndef TRIE_H
#define TRIE_H
#include <string>
#include <unordered_map>
#include <vector>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    std::vector<int> indices;
    TrieNode();
};

class Trie {
public:
    Trie();
    ~Trie();

    void insertKey(const std::string& word) const;
    std::vector<int> search(const std::string& word);
private:
    TrieNode* root;

    static void clear(const TrieNode* node);
};

#endif //TRIE_H
