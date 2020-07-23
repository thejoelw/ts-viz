#ifndef JWUTIL_MULTIDIMGRID_H
#define JWUTIL_MULTIDIMGRID_H

#include <array>
#include <algorithm>
#include <assert.h>

namespace jw_util
{

template <typename Type, unsigned int dims>
struct MultiDimGrid
{
public:
    typedef std::array<signed int, dims> Coord;

    MultiDimGrid()
        : min{{0}}
        , max{{0}}
    {
    }

    ~MultiDimGrid()
    {
        delete[] data;
    }

    template <typename Point>
    static Coord to_coord(Point point)
    {
        Coord coord;

        for (unsigned int i = 0; i < dims; i++)
        {
            if (std::is_floating_point<typename std::decay<decltype(point[i])>::type>::value)
            {
                coord[i] = floor(point[i]);
            }
            else
            {
                coord[i] = point[i];
            }
        }

        return coord;
    }

    template <typename Point>
    static Coord to_min(Point point)
    {
        return to_coord(point);
    }

    template <typename Point>
    static Coord to_max(Point point)
    {
        Coord res = to_coord(point);

        for (unsigned int i = 0; i < dims; i++)
        {
            res[i]++;
        }

        return res;
    }

    template <typename Point>
    Type &operator[](Point point) const
    {
        return operator[](to_coord(point));
    }

    Type &operator[](Coord coord) const
    {
        unsigned int index = 0;
        for (unsigned int i = 0; i < dims; i++)
        {
            assert(coord[i] >= min[i]);
            assert(coord[i] < max[i]);

            index *= max[i] - min[i];
            index += coord[i] - min[i];
        }

        return operator[](index);
    }

    Type &operator[](unsigned int index) const
    {
        assert(data);
        return data[index];
    }

    void constrain_coord(Coord &coord) const
    {
        for (unsigned int i = 0; i < dims; i++)
        {
            if (coord[i] < min[i]) {coord[i] = min[i];}
            if (coord[i] >= max[i]) {coord[i] = max[i] - 1;}
        }
    }

    template <typename Point>
    void expand_to(Point point)
    {
        expand_to(to_coord(point));
    }

    void expand_to(Coord coord)
    {
        Coord new_min = coord;
        Coord new_max = coord;

        for (unsigned int i = 0; i < dims; i++)
        {
            new_max[i]++;
        }

        expand_to(new_min, new_max);
    }

    void expand_to(Coord new_min, Coord new_max)
    {
        if (!data)
        {
            min = new_min;
            max = new_max;

            data = new Type[get_limit()];
            return;
        }

        unsigned int last_expand_dim = static_cast<unsigned int>(-1);

        for (unsigned int i = 0; i < dims; i++)
        {
            if (new_min[i] < min[i])
            {
                last_expand_dim = i;
            }
            else
            {
                new_min[i] = min[i];
            }

            if (new_max[i] > max[i])
            {
                last_expand_dim = i;
            }
            else
            {
                new_max[i] = max[i];
            }
        }

        if (last_expand_dim != static_cast<unsigned int>(-1))
        {
            update_min_max(new_min, new_max, last_expand_dim);
        }
    }

    void set_min_max(Coord new_min, Coord new_max)
    {
        if (!data)
        {
            min = new_min;
            max = new_max;

            data = new Type[get_limit()];
            return;
        }

        unsigned int last_expand_dim = dims;
        do
        {
            if (!last_expand_dim) {return;}
            last_expand_dim--;
        } while (new_min[last_expand_dim] == min[last_expand_dim] && new_max[last_expand_dim] == max[last_expand_dim]);

        update_min_max(new_min, new_max, last_expand_dim);
    }

    Coord get_min() const {return min;}
    Coord get_max() const {return max;}

    unsigned int get_limit() const
    {
        unsigned int size = 1;
        for (unsigned int i = 0; i < dims; i++)
        {
            size *= max[i] - min[i];
        }

        return size;
    }

private:
    // Min is inclusive of the lowest valid coordinate in each axis
    Coord min;

    // Max is exclusive of the highest valid coordinate in each axis
    Coord max;

    // For example, the points (-3, 4) and (5, -6) would have a min of (-3, -6) and a max of (6, 5)

    Type *data = 0;


    void update_min_max(Coord new_min, Coord new_max, unsigned int last_expand_dim)
    {
        Coord old_min = min;
        Coord old_max = max;

        min = new_min;
        max = new_max;

        unsigned int size = 1;
        unsigned int i = dims;
        while (--i > last_expand_dim)
        {
            assert(new_min[i] == old_min[i]);
            assert(new_max[i] == old_max[i]);
            size *= new_max[i] - new_min[i];
        }
        assert(i == last_expand_dim);

        unsigned int old_widths[dims];
        do
        {
            old_widths[i] = old_max[i] - old_min[i];

            new_min[i] *= size;
            new_max[i] *= size;
            old_min[i] *= size;
            old_max[i] *= size;

            size = new_max[i] - new_min[i];
        } while (i--);

        Type *new_data = new Type[size];

        Type *old_data_ref = data;
        Type *new_data_ref = new_data;
        copy_dim(last_expand_dim, old_min.data(), old_max.data(), new_min.data(), new_max.data(), old_widths, old_data_ref, new_data_ref);
        assert(new_data + size == new_data_ref);

        delete[] data;
        data = new_data;
    }

    static void copy_dim(unsigned int dim, signed int *old_min, signed int *old_max, signed int *new_min, signed int *new_max, unsigned int *widths, Type *&old_data, Type *&new_data)
    {
        assert(*old_min < *old_max);
        assert(*new_min < *new_max);
        
        signed int pre_pad = *old_min - *new_min;
        signed int post_pad = *new_max - *old_max;

        new_data += pre_pad;

        if (dim)
        {
            dim--;
            old_min++;
            old_max++;
            new_min++;
            new_max++;
            unsigned int width = *widths++;

            while (width--)
            {
                copy_dim(dim, old_min, old_max, new_min, new_max, widths, old_data, new_data);
            }
        }
        else
        {
            unsigned int old_size = *old_max - *old_min;
            std::move(old_data, old_data + old_size, new_data);
            old_data += old_size;
            new_data += old_size;
        }

        new_data += post_pad;
    }
};

}

#endif // JWUTIL_MULTIDIMGRID_H
