#ifndef JWUTIL_HUFFMANMODEL_H
#define JWUTIL_HUFFMANMODEL_H

#include <cassert>
#include <array>
#include <queue>
#include <limits.h>

#include "fastmath.h"

namespace jw_util
{

static constexpr unsigned int HuffmanModelDynamic = static_cast<unsigned int>(-1);

template <unsigned int outputs>
class HuffmanModel
{
public:
    class Builder
    {
    public:
        void add_output(unsigned int value, std::uint64_t freq)
        {
            nodes.push(new Node(freq, value));
        }

        void compile(HuffmanModel &model)
        {
            assert(!nodes.empty());
            alloc_data(model.read_tree, nodes.size() * 2 - 1);
            alloc_data(model.write_list, nodes.size());

            while (nodes.size() >= 2)
            {
                const Node *first = nodes.top();
                nodes.pop();

                const Node *second = nodes.top();
                nodes.pop();

                std::uint64_t freq_sum = first->freq + second->freq;

                // Catch possible overflow
                assert(freq_sum >= first->freq);
                assert(freq_sum >= second->freq);

                nodes.push(new Node(freq_sum, first, second));
            }

            unsigned int read_tree_pos = 0;
            descend(model, read_tree_pos, 0, 0, nodes.top());
            assert_at_end(model.read_tree, read_tree_pos);

            nodes.pop();
            assert(nodes.empty());
        }

        static void decompile(HuffmanModel &model)
        {
            free_data(model.read_tree);
            free_data(model.write_list);
        }

    private:
        struct Node
        {
            Node(std::uint64_t freq, unsigned int value)
                : freq(freq)
                , value(value)
                , children{0, 0}
            {}

            Node(std::uint64_t freq, const Node *child1, const Node *child2)
                : freq(freq)
                , children{child1, child2}
            {}

            std::uint64_t freq;
            unsigned int value;
            const Node *children[2];

            struct Comparator
            {
                bool operator() (const Node *a, const Node *b) const
                {
                    if (a->children[0] == 0 && b->children[0] == 0) {
                        assert(a->value != b->value);
                    }

                    if (a->freq != b->freq) {return a->freq > b->freq;}
                    else {return a->value > b->value;}
                }
            };
        };

        std::priority_queue<Node*, std::vector<Node*>, typename Node::Comparator> nodes;

        static void descend(HuffmanModel &model, unsigned int &read_tree_pos, unsigned int path, unsigned int path_bits, const Node *node)
        {
            unsigned int prev_read_tree_pos = read_tree_pos;
            assert_inside(model.read_tree, prev_read_tree_pos);
            read_tree_pos++;

            if (node->children[0])
            {
                // Branch
                assert(node->children[1]);

                descend(model, read_tree_pos, path | (0 << path_bits), path_bits + 1, node->children[0]);
                model.read_tree[prev_read_tree_pos] = read_tree_pos - prev_read_tree_pos;
                descend(model, read_tree_pos, path | (1 << path_bits), path_bits + 1, node->children[1]);
            }
            else
            {
                // Leaf
                assert(!node->children[1]);
                assert_inside(model.write_list, node->value);
                assert(path_bits < sizeof(unsigned int) * CHAR_BIT);

                model.read_tree[prev_read_tree_pos] = -node->value;
                model.write_list[node->value] = path | (1 << path_bits);
            }

            delete node;
        }
    };

    class Reader
    {
    public:
        Reader()
        {}

        Reader(const HuffmanModel &model)
            : ptr(get_data(model.read_tree))
        {}

        bool needs_bit() const
        {
            return *ptr > 0;
        }

        unsigned int get_result() const
        {
            assert(!needs_bit());
            return -*ptr;
        }

        void recv_bit(bool bit)
        {
            assert(needs_bit());
            ptr += bit ? *ptr : 1;
        }

    private:
        const signed int *ptr;
    };

    class Writer
    {
    public:
        Writer()
        {}

        Writer(const HuffmanModel &model, unsigned int value)
            : path(model.write_list[value])
        {}

        bool has_bit() const
        {
            assert(path >= 1);
            return path != 1;
        }

        bool get_bit() const
        {
            assert(has_bit());
            return path & 1;
        }

        void next_bit()
        {
            assert(has_bit());
            path >>= 1;
        }

        unsigned int num_bits_left() const
        {
            return FastMath::log2(path);
        }
        unsigned int get_remaining_bits() const
        {
            return path;
        }

    private:
        unsigned int path;
    };

private:
    typedef typename std::conditional<outputs == HuffmanModelDynamic, signed int*, std::array<signed int, outputs * 2 - 1>>::type ReadTreeType;
    typedef typename std::conditional<outputs == HuffmanModelDynamic, unsigned int*, std::array<unsigned int, outputs>>::type WriteListType;
    ReadTreeType read_tree;
    WriteListType write_list;

    template <typename DataType>
    static void alloc_data(DataType *&res, unsigned int count) {res = new DataType[count];}
    template <typename DataType, std::size_t size>
    static void alloc_data(std::array<DataType, size> &res, unsigned int count) {assert(count == size);}

    template <typename DataType>
    static DataType *get_data(DataType *res) {return res;}
    template <typename DataType, std::size_t size>
    static DataType *get_data(std::array<DataType, size> &res) {return res.data();}

    template <typename DataType>
    static const DataType *get_data(const DataType *res) {return res;}
    template <typename DataType, std::size_t size>
    static const DataType *get_data(const std::array<DataType, size> &res) {return res.data();}

    template <typename DataType>
    static void free_data(DataType *res) {delete[] res;}
    template <typename DataType, std::size_t size>
    static void free_data(std::array<DataType, size> &res) {}

    template <typename DataType>
    static void assert_inside(const DataType *res, unsigned int index) {}
    template <typename DataType, std::size_t size>
    static void assert_inside(const std::array<DataType, size> &res, unsigned int index) {assert(index < res.size());}

    template <typename DataType>
    static void assert_at_end(const DataType *res, unsigned int index) {}
    template <typename DataType, std::size_t size>
    static void assert_at_end(const std::array<DataType, size> &res, unsigned int index) {assert(index == res.size());}
};

}

#endif // JWUTIL_HUFFMANMODEL_H
