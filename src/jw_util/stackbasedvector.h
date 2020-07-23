#ifndef JWUTIL_STACKBASEDVECTOR_H
#define JWUTIL_STACKBASEDVECTOR_H

#include <vector>

// http://stackoverflow.com/questions/354442/looking-for-c-stl-like-vector-class-but-using-stack-storage
// http://stackoverflow.com/questions/8049657/stack-buffer-based-stl-allocator

namespace jw_util
{

template <typename DataType, std::size_t stack_size>
using StackBasedVector = std::vector<DataType>;

/*
template <typename Type, std::size_t stack_size>
class StackBasedVector
{
public:
	StackBasedVector()
	{
		// This class is not tested yet
		assert(false);
	}

	Type &operator[](std::size_t i)
	{
		if (i < stack_size)
		{
			// Use stack
			return stack[i];
		}
		else
		{
			// Use heap
			assert(i < size);
			return heap[i - stack_size];
		}
	}

	void push_back(Type &val)
	{
		if (size < stack_size)
		{
			// Use stack
			stack[size] = val;
		}
		else
		{
			// Use heap
			std::size_t heap_i = size - stack_size;
			if (heap_i >= alloc)
			{
				if (alloc)
				{
					Heap *new_heap = new Type[alloc * 2];
					std::copy_n(heap, new_heap, alloc);
					delete[] heap;
					heap = new_heap;
					alloc *= 2;
				}
				else
				{
					alloc = 16;
					heap = new Type[alloc];
				}
			}

			heap[heap_i] = val;
		}
		size++;
	}

	void pop_back()
	{
		size--;
	}

private:
	std::size_t size = 0;
	std::size_t alloc = 0;

	std::array<Type, stack_size> stack;
	Type *heap;
};
*/

}

#endif // JWUTIL_STACKBASEDVECTOR_H
