#include <utility>
#include <iterator>
#include <cstddef>

template <typename keyType, typename T>
class reverseIterator
{
private:
    keyType *key_ptr;
    T *val_ptr;
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::pair<keyType, T>;

    reverseIterator(keyType* key, T* val) : key_ptr(key), val_ptr(val) {}

    value_type operator*() const {
        return value_type{*key_ptr, *val_ptr};
    }

    reverseIterator& operator++()
    {
        --key_ptr;
        --val_ptr;
        return *this; 
    }

    reverseIterator& operator--()
    {
        ++key_ptr;
        ++val_ptr;
        return *this; 
    }

    bool operator==(const reverseIterator& other) const { return key_ptr == other.key_ptr || val_ptr == other.val_ptr; }
    bool operator!=(const reverseIterator& other) const { return !(*this == other); }
};

template <typename keyType, typename T>
class Iterator
{
private:
    keyType *key_ptr;
    T *val_ptr;
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = std::pair<keyType, T>;
    using reference         = void;
    using pointer           = void;

    Iterator() = delete;
    Iterator(keyType* key, T* val) : key_ptr(key), val_ptr(val) {}

    value_type operator*() const {
        return value_type{*key_ptr, *val_ptr};
    }

    Iterator& operator++()
    {
        ++key_ptr;
        ++val_ptr;
        return *this; 
    }

    Iterator& operator--()
    {
        --key_ptr;
        --val_ptr;
        return *this; 
    }

    difference_type operator+(const Iterator& other) const {
            return key_ptr + other.key_ptr || val_ptr + other.val_ptr;
    }

    difference_type operator-(const Iterator& other) const {
         return key_ptr - other.key_ptr || val_ptr - other.val_ptr ;
    }

    bool operator==(const Iterator& other) const { return key_ptr == other.key_ptr || val_ptr == other.val_ptr; }
    bool operator!=(const Iterator& other) const { return !(*this == other); }
};

template <typename keyType, typename T>
class flatMap
{
private:
    keyType* keys;
    T* values; 
    int capacity;
    int size;
public:

    flatMap ()
    {
        capacity = 1;
        size = 0;
        keys = new keyType[1];
        values = new T[1];
    }

    ~flatMap ()
    {
        delete[] keys;
        delete[] values;
    }

    flatMap(const flatMap &other)
    {
        capacity = other.capacity;
        size = other.size;
        keys = new keyType[capacity];
        values = new T[capacity];
        for(int i = 0; i < size; i++)
        {
            keys[i] = other.keys[i];
            values[i] = other.values[i];
        }
    }

    flatMap& operator=(const flatMap &other)
    {
        if(this != &other)
        {
            clear();
            capacity = other.capacity;
            size = other.size;
            keys = new keyType[capacity];
            values = new T[capacity];
            for(int i = 0; i < size; i++)
            {
                keys[i] = other.keys[i];
                values[i] = other.values[i];
            }
        }
        return *this;
    }

    flatMap(flatMap &&other)
    {
        capacity = other.capacity;
        size = other.size;
        keys = other.keys; 
        values = other.values;

        other.capacity = 0;
        other.size = 0;
        other.keys = nullptr;
        other.values = nullptr;
        
    }

    flatMap& operator=(flatMap &&other)
    {
        if (this != &other)
        {
            clear();
            keys = other.keys;
            values = other.values;
            size = other.size;
            capacity = other.capacity;

            other.keys = nullptr;
            other.values = nullptr;
            other.size = 0;
            other.capacity = 0; 
        }
        return *this;
    }

    void insert(keyType k, T val)
    {
        int pos = 0; 
        while (pos < size && keys[pos] < k)
        {
            pos++;
        }

        if (pos < size && keys[pos] == k)
        {
            return; 
        }

        if (capacity == size)
        {
            if (capacity == 0) { capacity++ ;}
            keyType* temp_keys = new keyType[2 * capacity];
            T* temp_values = new T[2 * capacity];

            for (int n = 0; n < size; n++) {
                temp_keys[n] = keys[n];
                temp_values[n] = values[n];
            }

            delete[] keys;
            delete[] values;
            capacity *= 2;
            keys = temp_keys;
            values = temp_values;
        }

        for (int j = size; j > pos; j-- )
        {
            keys[j] = keys[j - 1];
            values[j] = values[j - 1];
        }

        keys[pos] = k;
        values[pos] = val;
        size++;
    }

    T& operator[](keyType k)
    {   
        int pos = 0; 
        while (pos < size && keys[pos] < k)
        {
            pos++;
        } 

        if (pos < size && keys[pos] == k)
        {
            return values[pos];
        }

        insert(k,T());
        return values[pos];
    }
    

    int get_size() const { return size; }
    int get_capacity() const { return capacity; }
    bool empty() const { return size == 0;}

    bool contains(keyType k) const 
    { 
        int pos = 0; 
        while (pos < size)
        {
            if (keys[pos] == k)
            {
                return true; 
            }
            pos++;
        }
        return false;
    }

    void clear()
    {
        delete[] keys;
        delete[] values;
        keys = nullptr;
        values = nullptr;
        capacity = 0;
        size = 0;
    }

    Iterator<keyType, T> begin() const { return Iterator<keyType, T>(keys, values); }
    Iterator<keyType, T> end() const { return Iterator<keyType, T>(keys + size, values + size); }
    reverseIterator<keyType, T> rbegin() const { return reverseIterator<keyType, T>(keys + size - 1, values + size - 1); }
    reverseIterator<keyType, T> rend() const { return reverseIterator<keyType, T>(keys, values); }
};
