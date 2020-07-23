#pragma once

#include <cstdint>
#include <limits.h>
#include <type_traits>
#include <assert.h>

namespace jw_util {

template <unsigned int size_>
class Bitset {
public:
    static constexpr unsigned int size = size_;

    typedef typename std::conditional<size <= 8,
        std::uint_fast8_t,
        typename std::conditional<size <= 16,
            std::uint_fast16_t,
            typename std::conditional<size <= 32,
                std::uint_fast32_t,
                std::uint_fast64_t
        >::type>::type>::type WordType;

    static constexpr unsigned int wordBits = sizeof(WordType) * CHAR_BIT;
    static constexpr unsigned int numWords = (size + wordBits - 1) / wordBits;

    static_assert((wordBits & (wordBits - 1)) == 0, "wordBits must be a power of 2");

    class ValueIterator {
    public:
        ValueIterator(Bitset &bitset)
            : bitset(bitset)
        {
            findOne();
        }

        bool has() const {
            return curValue < size;
        }

        unsigned int get() const {
            return curValue;
        }

        void advance() {
            curValue++;
            findOne();
        }

        void flip() {
            bitset.words[curValue / wordBits] ^= static_cast<WordType>(1) << (curValue % wordBits);
        }

    private:
        Bitset &bitset;
        unsigned int curValue = 0;

        void findOne() {
            while (curValue < size) {
                WordType rest = bitset.words[curValue / wordBits] >> (curValue % wordBits);
                if (rest) {
                    curValue += getLsbIndex(rest);
                    break;
                } else {
                    curValue &= ~(wordBits - 1);
                    curValue += wordBits;

                    assert(curValue % wordBits == 0);
                }
            }
        }
    };

    template <bool value>
    void fill() {
        for (unsigned int i = 0; i < numWords; i++) {
            words[i] = value ? ~static_cast<WordType>(0) : 0;
        }

        if (value) {
            maskLastWord();
        }
    }

    template <bool value>
    void set(unsigned int index) {
        assert(index < size);
        if (value) {
            words[index / wordBits] |= static_cast<WordType>(1) << (index % wordBits);
        } else {
            words[index / wordBits] &= ~(static_cast<WordType>(1) << (index % wordBits));
        }
    }

    bool get(unsigned int index) const {
        assert(index < size);
        return (words[index / wordBits] >> (index % wordBits)) & 0x1;
    }

    unsigned int count() const {
        unsigned int count = 0;

        for (unsigned int i = 0; i < numWords; i++) {
            count += getPopcount(words[i]);
        }

        return count;
    }

    bool none() const {
        for (unsigned int i = 0; i < numWords; i++) {
            if (words[i]) {
                return false;
            }
        }
        return true;
    }

    Bitset<size> operator~() const {
        Bitset<size> res;
        for (unsigned int i = 0; i < numWords; i++) {
            res.words[i] = ~words[i];
        }
        res.maskLastWord();
        return res;
    }

    Bitset<size> operator&(const Bitset<size> &other) const {
        Bitset<size> res;
        for (unsigned int i = 0; i < numWords; i++) {
            res.words[i] = words[i] & other.words[i];
        }
        return res;
    }

    Bitset<size> operator|(const Bitset<size> &other) const {
        Bitset<size> res;
        for (unsigned int i = 0; i < numWords; i++) {
            res.words[i] = words[i] | other.words[i];
        }
        return res;
    }

    Bitset<size> operator^(const Bitset<size> &other) const {
        Bitset<size> res;
        for (unsigned int i = 0; i < numWords; i++) {
            res.words[i] = words[i] ^ other.words[i];
        }
        return res;
    }

    bool operator==(const Bitset<size> &other) const {
        for (unsigned int i = 0; i < numWords; i++) {
            if (words[i] != other.words[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Bitset<size> &other) const {
        return !(*this == other);
    }

    void maskLastWord() {
        words[numWords - 1] &= (static_cast<WordType>(1) << (size % wordBits)) - 1;
    }

    static unsigned int getLsbIndex(unsigned char word) { return getLsbIndex(static_cast<unsigned int>(word)); }
    static unsigned int getLsbIndex(unsigned short word) { return getLsbIndex(static_cast<unsigned int>(word)); }
    static unsigned int getLsbIndex(unsigned int word) { return __builtin_ctz(word); }
    static unsigned int getLsbIndex(unsigned long word) { return __builtin_ctzl(word); }
    static unsigned int getLsbIndex(unsigned long long word) { return __builtin_ctzll(word); }

    static unsigned int getPopcount(unsigned char word) { return getPopcount(static_cast<unsigned int>(word)); }
    static unsigned int getPopcount(unsigned short word) { return getPopcount(static_cast<unsigned int>(word)); }
    static unsigned int getPopcount(unsigned int word) { return __builtin_popcount(word); }
    static unsigned int getPopcount(unsigned long word) { return __builtin_popcountl(word); }
    static unsigned int getPopcount(unsigned long long word) { return __builtin_popcountll(word); }

    WordType words[numWords];
};

}
