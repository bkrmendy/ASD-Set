#pragma once
#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>

template <typename Key, size_t N = 7>
class ADS_set {
public:
    class Iterator;
    using value_type = Key;
    using key_type = Key;
    using reference = key_type&;
    using const_reference = const key_type&;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = Iterator;
    using const_iterator = Iterator;
    using key_compare = std::less<key_type>;   // B+-Tree
    using key_equal = std::equal_to<key_type>; // Hashing
    using hasher = std::hash<key_type>;        // Hashing
private:

    struct Node {
        key_type key;
        Node* next;
    };

    struct list {
        Node* front = nullptr;
    };

    list* table{ nullptr };
    size_type size_{ 0 };
    size_type table_size { 0 };
    float max_lf{ 0.7 };

    size_type hash_of(const key_type& key) const {
        return hasher{}(key) % table_size;
    }

    void reserve(size_type requested_size);
    void rehash(size_type requested_table_size);
    Node* insert_(const key_type& key);
    Node* find_(const key_type& key) const;

public:

    ADS_set() {
        rehash(N);
    }

    ADS_set(std::initializer_list<key_type> ilist) {
        rehash(N);
        insert(ilist);
    }

    template<typename InputIt> ADS_set(InputIt first, InputIt last) {
        rehash(N);
        insert(first, last);
    }

    ADS_set(const ADS_set& other) {
        rehash(other.table_size);

        auto current = other.begin();
        while (current != other.end()) {
            insert_(*current);
            current++;
        }

        this->size_ = other.size_;
        this->max_lf = other.max_lf;
    }

    ~ADS_set() {
        clear();
        delete[] table;
    }

    ADS_set& operator=(const ADS_set& other) {
        if (this == &other) {
            return *this;
        }

        ADS_set tmp{other};
        swap(tmp);
        return *this;
    }

    ADS_set& operator=(std::initializer_list<key_type> ilist) {
        ADS_set tmp{ilist};
        swap(tmp);
        return *this;
    }

    size_type size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    size_type count(const key_type& key) const {
        auto it = find(key);
        return it == this->end()
            ? 0
            : 1;
    }

    iterator find(const key_type& key) const {
        Node* ptr = find_(key);
        if (ptr != nullptr) {
            size_type idx{hash_of(key) };
            return Iterator{ptr, &table[idx], &table[table_size]};
        }
        return this->end();
    }

    void clear() {
        for (size_type idx = 0; idx < table_size; idx++) {
            Node* current = table[idx].front;
            while (current != nullptr) {
                current = current->next;
                delete current;
            }
            table[idx].front = nullptr;
        }
        this->size_ = 0;
    }

    void swap(ADS_set& other) {
        std::swap(this->size_, other.size_);
        std::swap(this->table, other.table);
        std::swap(this->table_size, other.table_size);
        std::swap(this->max_lf, other.max_lf);
    }

    void insert(std::initializer_list<key_type> ilist) {
        insert(std::begin(ilist), std::end(ilist));
    }

    std::pair<iterator, bool> insert(const key_type& key) {
        auto i = find(key);
        if (i != this->end()) {
            return { i, false };
        }
        reserve(size_ + 1);
        auto it = insert_(key);
        size_type idx{hash_of(key) };
        return { Iterator{ it, &table[idx], &table[table_size] }, true };
    }

    template<typename InputIt> void insert(InputIt first, InputIt last);

    size_type erase(const key_type& key) {
        size_type idx{hash_of(key) };
        Node* temp = table[idx].front;
        Node* previous = nullptr;

        while (temp != nullptr && not key_equal{}(temp->key, key)) {
            previous = temp;
            temp = temp->next;
        }
        if (temp == nullptr) { // item not found
            return 0;
        }
        if (previous == nullptr) { // meaning its the 1st node in the list
            table[idx].front = temp->next;
            delete temp;
            --size_;
            return 1;
        }
        previous->next = temp->next; // found element is at any other place in the list
        delete temp;
        --size_;
        return 1;
    }

    const_iterator begin() const {
        return Iterator{&table[0], &table[table_size]};
    }

    const_iterator end() const {
        return Iterator{&table[table_size], &table[table_size]};
    }

    void dump(std::ostream& o = std::cerr) const;

    friend bool operator==(const ADS_set& lhs, const ADS_set& rhs) {
        if (lhs.size_ != rhs.size_) {
            return false;
        }

        for (const auto &k : rhs) {
            if (lhs.count(k) < 1) {
                return false;
            }
        }
        return true;
    }

    friend bool operator!=(const ADS_set& lhs, const ADS_set& rhs) {
        return !(lhs == rhs);
    }
};

template <typename Key, size_t N>
typename ADS_set<Key, N>::Node* ADS_set<Key, N>::insert_(const key_type& key) {
    size_type idx{hash_of(key) };
    Node* temp = new Node{};
    temp->key = key;
    if (table[idx].front == nullptr) {
        temp->next = nullptr;
        table[idx].front = temp;
    } else {
        temp->next = table[idx].front;
        table[idx].front = temp;
    }
    ++size_;
    return table[idx].front;
}

template <typename Key, size_t N>
typename ADS_set<Key, N>::Node* ADS_set<Key, N>::find_(const key_type& key) const {
    size_type idx{hash_of(key) };
    Node* temp = table[idx].front;
    while (temp) {
        if (key_equal{}(temp->key, key)) {
            return temp;
        }
        temp = temp->next;
    }
    return nullptr;
}

template <typename Key, size_t N>
template<typename InputIt> void ADS_set<Key, N>::insert(InputIt first, InputIt last) {
    for (auto it{ first }; it != last; ++it) {
        if (count(*it) == 0) {
            reserve(size_ + 1);
            insert_(*it);
        }
    }
}

template <typename Key, size_t N>
void ADS_set<Key, N>::reserve(size_type requested_size) {
    if (requested_size > table_size * max_lf) {
        size_type new_table_size{ table_size };

        do {
            new_table_size = new_table_size * 2 + 1;
        } while (requested_size > new_table_size * max_lf);

        rehash(new_table_size);
    }
}

template <typename Key, size_t N>
void ADS_set<Key, N>::rehash(size_type requested_table_size) {
    size_type old_table_size = this->table_size;
    size_type new_table_size = std::max(N, std::max(requested_table_size, size_type(size_ / max_lf)));

    list* new_table{ new list[new_table_size] };

    list* old_table{ table };

    this->size_ = 0;
    this->table_size = new_table_size;
    this->table = new_table;

    for (size_type idx = 0; idx < old_table_size; ++idx) {
        Node* temp = old_table[idx].front;
        while (temp != nullptr) {
            auto key = temp->key;
            auto next = temp->next;

            delete temp;

            insert_(key);
            temp = next;
        }
    }

    delete[] old_table;
}

template <typename Key, size_t N>
void ADS_set<Key, N>::dump(std::ostream& o) const {
    o << "curr_size = " << size_ << " table_size = " << N << "\n";
    for (size_type idx{ 0 }; idx < table_size; ++idx) {
        o << idx << ": ";
        Node* temp = table[idx].front;
        while (temp != nullptr) {
            o << " -> " << temp->key;
            temp = temp->next;
        }
        o << "\n";
    }
}

template <typename Key, size_t N>
class ADS_set<Key, N>::Iterator {
public:
    using value_type = Key;
    using difference_type = std::ptrdiff_t;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator_category = std::forward_iterator_tag;
private:
    Node* listPos;
    list* tablePos;
    list* end;

    void skipToNext() {
        if (tablePos == end) {
            return;
        }

        if (listPos->next != nullptr) {
            listPos = listPos->next;
            return;
        }

        listPos = nullptr;
        tablePos++;
        while (tablePos != end) {
            if (tablePos->front != nullptr) {
                listPos = tablePos->front;
                return;
            }
            tablePos++;
        }
    }

    void skipToFirst() {
        while (tablePos != end) {
            if (tablePos->front != nullptr) {
                listPos = tablePos->front;
                return;
            }
            tablePos++;
        }
        listPos = nullptr;
    }

public:

    Iterator() : listPos{ nullptr }, tablePos{ nullptr }, end{ nullptr } { }

    Iterator(Node* listPos, list* tablePos, list* end) : listPos{ listPos }, tablePos{ tablePos }, end{ end } { }

    Iterator(list* tablePos, list* end) : listPos{nullptr}, tablePos{tablePos}, end{end} {
        skipToFirst();
    }

    reference operator*() const {
        return listPos->key;
    }

    pointer operator->() const {
        return &listPos->key;
    }

    Iterator& operator++() {
        skipToNext();
        return *this;
    }

    Iterator operator++(int) {
        auto rc{ *this };
        skipToNext();
        return rc;
    }

    friend bool operator==(const Iterator& lhs, const Iterator& rhs) {
        return (lhs.tablePos == rhs.tablePos
                and lhs.listPos == rhs.listPos
                and lhs.end == rhs.end);
    }

    friend bool operator!=(const Iterator& lhs, const Iterator& rhs) {
        return !(lhs == rhs);
    }
};
