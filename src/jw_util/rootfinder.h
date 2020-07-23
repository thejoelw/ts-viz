#ifndef JWUTIL_ROOTFINDER_H
#define JWUTIL_ROOTFINDER_H

#include "funcderivatives.h"
#include "multidimmatrix.h"

namespace
{
    template <unsigned int size>
    float determinant(const jw_util::MultiDimMatrix<size, 2> &mat)
    {
        float res = 0.0f;

        jw_util::MultiDimMatrix<size-1, 2> minor;
        for (unsigned int i = 1; i < size; i++)
        {
            for (unsigned int j = 1; j < size; j++)
            {
                minor[i-1][j-1] = static_cast<float>(mat[i][j]);
            }
        }

        for (unsigned int i = 0; i < size; i++)
        {
            float prod = mat[i][0] * determinant<size-1>(minor);
            if (i % 2)
            {
                res -= prod;
            }
            else
            {
                res += prod;
            }

            if (i != size - 1)
            {
                for (unsigned int j = 1; j < size; j++)
                {
                    minor[i][j-1] = mat[i][j];
                }
            }
        }

        return res;
    }

    template <>
    float determinant<0>(const jw_util::MultiDimMatrix<0, 2> &mat)
    {
        return 0.0f;
    }
    template <>
    float determinant<1>(const jw_util::MultiDimMatrix<1, 2> &mat)
    {
        return mat[0][0];
    }
    template <>
    float determinant<2>(const jw_util::MultiDimMatrix<2, 2> &mat)
    {
        return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    }
}

namespace jw_util
{

template <unsigned int dims>
class RootFinder
{
    // Matrices here are indexed by [col][row]

public:
    template <typename ResultType>
    static ResultType solve_newton(const FuncDerivatives<dims, 1> *equs[dims])
    {
        MultiDimMatrix<dims, 1> value;
        MultiDimMatrix<dims, 2> jacobian;
        for (unsigned int i = 0; i < dims; i++)
        {
            const FuncDerivatives<dims, 1> &equ = *equs[i];
            value[i] = equ.template get_deriv<0>();
            jacobian[i] = equ.template get_deriv<1>();
        }

        float inv_det = 1.0f / determinant<dims>(jacobian);

        MultiDimMatrix<dims, 1> a;
        solve_matrix_vec<dims>(jacobian, value, inv_det, a);

        ResultType result;
        for (unsigned int i = 0; i < dims; i++)
        {
            result[i] = -a[i];
        }
        return result;
    }

    template <typename ResultType>
    static ResultType solve_halley(const FuncDerivatives<dims, 2> *equs[dims])
    {
        MultiDimMatrix<dims, 1> value;
        MultiDimMatrix<dims, 2> jacobian;
        for (unsigned int i = 0; i < dims; i++)
        {
            const FuncDerivatives<dims, 2> &equ = *equs[i];
            value[i] = equ.template get_deriv<0>();
            jacobian[i] = equ.template get_deriv<1>();
        }

        float inv_det = 1.0f / determinant<dims>(jacobian);

        MultiDimMatrix<dims, 1> a;
        solve_matrix_vec<dims>(jacobian, value, inv_det, a);

        MultiDimMatrix<dims, 2> hess_prod;
        for (unsigned int i = 0; i < dims; i++)
        {
            vec_matrix_prod<dims>(a, equs[i]->template get_deriv<2>(), hess_prod[i]);
        }

        MultiDimMatrix<dims, 1> t;
        matrix_vec_prod<dims>(hess_prod, a, t);

        MultiDimMatrix<dims, 1> b;
        solve_matrix_vec<dims>(jacobian, t, inv_det, b);

        ResultType result;
        for (unsigned int i = 0; i < dims; i++)
        {
            float res = a[i] * a[i] / (0.5f * b[i] - a[i]);
            result[i] = res;
        }
        return result;
    }

    template <typename ResultType>
    static ResultType find_solution_2nd_order(const FuncDerivatives<dims, 2> &func)
    {
        // Finds a single root of a second-order function, in the direction of the gradient vector

        /*
        // Start with density, grad, hess
        // Calculate solution vector using a second-order root finding approximation:
        grad2 = grad / grad.norm() * hess
        d0 = density
        d1 = grad.norm()
        d2 = grad2.norm()
        t = (sqrt(d1 * d1 - 2 * d0 * d2) - d1) / d2
        move = grad / grad.norm() * t

        # Simplified
        d0 = density
        d1 = grad * grad
        d2 = (grad * hess).norm() * sqrt(d1)
        t = (sqrt(d1 * d1 - 2 * d0 * d2) - d1) / d2
        move = grad * t
        */

        MultiDimMatrix<dims, 1> grad_hess;
        vec_matrix_prod(func.template get_deriv<1>(), func.template get_deriv<2>(), grad_hess);

        float d0 = func.template get_deriv<0>();
        float d1 = vec_length_sq(func.template get_deriv<1>());
        float d2 = FastMath::sqrt<2, FastMath::SqrtImpEither>(vec_length_sq(grad_hess) * d1);

        float disc = d1 * d1 - 2.0f * d0 * d2;
        float t;
        if (d2 < 0.001f || disc < 0.0f)
        {
            // First-order approximation
            t = -(d0 / d1);
        }
        else
        {
            // Second-order approximation
            t = (FastMath::sqrt<2, FastMath::SqrtImpEither>(disc) - d1) / d2;
        }

        ResultType result;
        for (unsigned int i = 0; i < dims; i++)
        {
            result[i] = func.template get_deriv<1>()[i] * t;
        }
        return result;
    }


private:
    template <unsigned int size>
    static void matrix_vec_prod(const MultiDimMatrix<size, 2> &mat, const MultiDimMatrix<size, 1> &vec, MultiDimMatrix<size, 1> &result)
    {
        // Multiply a square matrix by a column vector

        for (unsigned int i = 0; i < size; i++)
        {
            result[i] = 0.0f;

            for (unsigned int j = 0; j < size; j++)
            {
                result[i] += mat[j][i] * vec[j];
            }
        }
    }

    template <unsigned int size>
    static void vec_matrix_prod(const MultiDimMatrix<size, 1> &vec, const MultiDimMatrix<size, 2> &mat, MultiDimMatrix<size, 1> &result)
    {
        // Multiply a row vector by a square matrix

        for (unsigned int i = 0; i < size; i++)
        {
            result[i] = 0.0f;

            for (unsigned int j = 0; j < size; j++)
            {
                result[i] += mat[i][j] * vec[j];
            }
        }
    }

    template <unsigned int size>
    static float determinant(const MultiDimMatrix<size, 2> &mat)
    {
        return ::determinant<size>(mat);
    }

    template <unsigned int size>
    static float vec_length_sq(const MultiDimMatrix<size, 1> &mat)
    {
        float res = 0.0f;
        for (unsigned int i = 0; i < size; i++)
        {
            res += mat[i] * mat[i];
        }
        return res;
    }

    template <unsigned int size>
    static void solve_matrix_vec(const MultiDimMatrix<size, 2> &mat, const MultiDimMatrix<size, 1> &equals, float inv_determinant, MultiDimMatrix<size, 1> &result)
    {
        // TODO: implement for non-3-dimensional sizes
        static_assert(size == 3, "RootFinder::solve_matrix() not yet implemented for matrices of size other than 3");

        result[0] = -((mat[2][1] * mat[1][2] - mat[1][1] * mat[2][2]) * equals[0] - (equals[2] * mat[2][1] - equals[1] * mat[2][2]) * mat[1][0] + (equals[2] * mat[1][1] - equals[1] * mat[1][2]) * mat[2][0]) * inv_determinant;
        result[1] = ((mat[2][1] * mat[0][2] - mat[0][1] * mat[2][2]) * equals[0] - (equals[2] * mat[2][1] - equals[1] * mat[2][2]) * mat[0][0] + (equals[2] * mat[0][1] - equals[1] * mat[0][2]) * mat[2][0]) * inv_determinant;
        result[2] = -((mat[1][1] * mat[0][2] - mat[0][1] * mat[1][2]) * equals[0] - (equals[2] * mat[1][1] - equals[1] * mat[1][2]) * mat[0][0] + (equals[2] * mat[0][1] - equals[1] * mat[0][2]) * mat[1][0]) * inv_determinant;
    }

    template <unsigned int size>
    static void solve_vec_matrix(const MultiDimMatrix<size, 2> &mat, const MultiDimMatrix<size, 1> &equals, float inv_determinant, MultiDimMatrix<size, 1> &result)
    {
        // TODO: implement for non-3-dimensional sizes
        static_assert(size == 3, "RootFinder::solve_matrix() not yet implemented for matrices of size other than 3");

        result[0] = ((mat[1][1] * mat[2][2] - mat[1][2] * mat[2][1]) * equals[0] - (equals[1] * mat[2][2] - equals[2] * mat[1][2]) * mat[0][1] - (equals[2] * mat[1][1] - equals[1] * mat[2][1]) * mat[0][2]) * inv_determinant;
        result[1] = -((mat[2][2] * mat[1][0] - mat[1][2] * mat[2][0]) * equals[0] - (equals[1] * mat[2][2] - equals[2] * mat[1][2]) * mat[0][0] - (equals[2] * mat[1][0] - equals[1] * mat[2][0]) * mat[0][2]) * inv_determinant;
        result[2] = -((mat[1][1] * mat[2][0] - mat[1][0] * mat[2][1]) * equals[0] - (equals[2] * mat[1][1] - equals[1] * mat[2][1]) * mat[0][0] + (equals[2] * mat[1][0] - equals[1] * mat[2][0]) * mat[0][1]) * inv_determinant;
    }

/*
public:

    class Quadric
    {
        friend class SpaceRootFinder;

        Quadric()
            : value(0.0f)
            , jacobian(glm::vec3(0.0f))
            , hessian(glm::mat3x3(0.0f))
        {}

        float value;
        glm::vec3 jacobian;
        glm::mat3x3 hessian;
    };

    template <unsigned int equ_id>
    Quadric &get_equ()
    {
        static_assert(equ_id < 3, "Template parameter for SpaceRootFinder::get_equ must be less than 3");
        return equs[equ_id];
    }

private:
    Quadric equs[3];

    glm::vec3 solve()
    {
        // p^T is a row vector
        // p is a column vector - default (R^m)

        // Halleys method:
        glm::vec3 val = glm::vec3(equs[0].value, equs[1].value, equs[2].value);
        glm::mat3x3 jac = glm::mat3x3(equs[0].jacobian, equs[1].jacobian, equs[2].jacobian);
        glm::mat3x3 inv_jac = glm::inverse(jac);
        glm::vec3 a = (inv_jac * val);
        glm::mat3x3 hess_prod = glm::mat3x3(a * equs[0].hessian, a * equs[1].hessian, a * equs[2].hessian);
        // TODO: put parenthesis around (hess_prod * a), make sure equivalent
        glm::vec3 b = inv_jac * hess_prod * a;
        glm::vec3 move = a * a / (-a + 0.5f * b);
        return move;
*/
        /*
        // Halleys method variation:
        glm::mat3x3 jac = glm::mat3x3(equ.jac[0], equ.jac[1], equ.jac[2]);
        glm::mat3x3 inv_jac = glm::inverse(jac);
        glm::vec3 a = -(inv_jac * equ.val);
        glm::mat3x3 hess_prod = glm::mat3x3(a * equ.hess[0], a * equ.hess[1], a * equ.hess[2]);
        glm::vec3 b = glm::mat3x3(1.0f) + 0.5f * inv_jac * hess_prod;
        glm::vec3 move = glm::inverse(b) * a;
        return move;
        */
    /*
    }
    */
};

}

#endif // JWUTIL_ROOTFINDER_H
