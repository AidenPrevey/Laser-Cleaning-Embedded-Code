#pragma once

#include <array>
#include <cstdint>

/**
 * @struct Coefficients
 * @brief Represents the coefficients used in a discrete filter.
 *
 * This structure holds two sets of coefficients:
 * - `naturalResponseCoefficients`: Coefficients related to the natural response of the system.
 * - `forcedResponseCoefficients`: Coefficients related to the forced response of the system.
 *
 * @tparam SIZE The size of the coefficient arrays.
 * @tparam T The data type of the coefficients (e.g., float, double).
 */
template <uint8_t SIZE, typename T = float>
struct Coefficients
{
    std::array<T, SIZE> naturalResponseCoefficients;
    std::array<T, SIZE> forcedResponseCoefficients;
};

/**
 * @brief DiscreteFilter class implements a discrete-time filter using the finite difference
 * equation.
 * @tparam SIZE The size of the filter coefficients.
 *
 * This class provides methods to filter input data using the finite difference equation and
 * maintain the internal state of the filter.
 */
template <uint8_t SIZE, typename T = float>
class DiscreteFilter
{
public:
    /**
     * @brief Constructor for the DiscreteFilter class.
     * @param [in] naturalResponseCoefficients The coefficients for the natural response (a).
     * @param [in] forcedResponseCoefficients The coefficients for the forced response (b).
     *
     * This constructor initializes the filter with the given coefficients and resets the filter
     * state to zero.
     */
    DiscreteFilter(
        std::array<T, SIZE> naturalResponseCoefficients,
        std::array<T, SIZE> forcedResponseCoefficients)
        : naturalResponseCoefficients(naturalResponseCoefficients),
          forcedResponseCoefficients(forcedResponseCoefficients)
    {
        reset();
    }

    DiscreteFilter(const Coefficients<SIZE, T> coefficients)
        : naturalResponseCoefficients(coefficients.naturalResponseCoefficients),
          forcedResponseCoefficients(coefficients.forcedResponseCoefficients)
    {
        reset();
    }

    /**
     * @brief Filters the input data using the finite difference equation.
     * @param [in] dat The input data to be filtered.
     * @return The filtered output data.
     *
     * This function implements a discrete-time filter using the finite difference equation.
     * It updates the internal state of the filter based on the input data and returns the
     * filtered output.
     *
     * \f$ y(n)=\frac{1}{a_{0}}\left[\sum_{\kappa=0}^{M} b_{\kappa}x(n-\kappa)-\sum_{k=1}^{N}
     * a_{k}y(n-k)\right]. \qquad{(2)} \f$
     *
     */
    T filterData(float dat)
    {
        for (int i = SIZE - 1; i > 0; i--)
        {
            forcedResponse[i] = forcedResponse[i - 1];
        }
        forcedResponse[0] = dat;

        float sum = 0;
        // Sum of forced response coefficients multiplied by the forced response X(n-k)
        // (previous input data)
        for (int i = 0; i < SIZE; i++)
        {
            sum += forcedResponseCoefficients[i] * forcedResponse[i];
        }
        // Sum of natural response coefficients multiplied by the natural response Y(n-k)
        // (previous output data)
        for (int i = 0; i < SIZE - 1; i++)
        {
            sum -= naturalResponseCoefficients[i + 1] * naturalResponse[i];
        }
        // Apply the 1/a_0 scaling to the output
        sum /= naturalResponseCoefficients[0];

        // Shift the natural response array to make room for the new output
        for (int i = SIZE - 1; i > 0; i--)
        {
            naturalResponse[i] = naturalResponse[i - 1];
        }

        naturalResponse[0] = sum;

        return naturalResponse[0];
    }

    /** @brief Returns the last filtered value*/
    T getLastFiltered() { return naturalResponse[0]; }

    /** @brief Resets the filter's state to zero, keeps the coefficients  */

    T reset()
    {
        // Reset the filter state to zero
        naturalResponse.fill(0.0f);
        forcedResponse.fill(0.0f);
        return 0.0f;
    }

    // fill it with the value given (good for init a DC gain)
    T fill(T value)
    {
        naturalResponse.fill(value);
        forcedResponse.fill(value);
        return value;
    }

    void setCoefficients(Coefficients<SIZE, T> coe)
    {
        this->naturalResponseCoefficients = coe.naturalResponseCoefficients;
        this->forcedResponseCoefficients  = coe.forcedResponseCoefficients;
    }

    void setCoefficients(
        std::array<T, SIZE> naturalResponseCoefficients,
        std::array<T, SIZE> forcedResponseCoefficients)
    {
        this->naturalResponseCoefficients = naturalResponseCoefficients;
        this->forcedResponseCoefficients  = forcedResponseCoefficients;
    }

private:
    std::array<T, SIZE> naturalResponseCoefficients;
    std::array<T, SIZE> forcedResponseCoefficients;
    std::array<T, SIZE> naturalResponse;
    std::array<T, SIZE> forcedResponse;
};