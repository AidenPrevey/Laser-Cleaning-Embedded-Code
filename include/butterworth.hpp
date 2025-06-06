#pragma once

#ifndef butterworth_h
#define butterworth_h

#include <array>
#include <cmath>
#include <complex>
#include <cstdint>

#include "discrete_filter.hpp"

/**
 * @brief Implementation of Butterworth filter design in the discrete domain.
 *
 * This header file provides a implementation of Butterworth filters,
 * including low-pass, high-pass, band-pass, and band-stop filters. The Butterworth
 * filter is known for its maximally flat frequency response in the passband, making
 * it ideal for applications requiring minimal signal distortion.
 *
 * The implementation includes:
 * - Conversion of poles and zeros from the Laplace domain to the Z domain using
 *   the bilinear transform.
 * - Expansion of polynomial coefficients from a set of poles or zeros.
 * - Evaluation of the frequency response of the filter at a given frequency.
 * - A templated Butterworth filter class for designing filters of arbitrary order
 *   and type.
 *
 * The design process includes pre-warping of frequencies for the bilinear transform,
 * generation of prototype poles, and scaling of coefficients.
 *
 * The following transforms map a lowpass prototype into other filter types (s-domain):
 *
 * - **Lowpass to Lowpass**:
 *   \f$ s \rightarrow \frac{s}{\Omega_c} \f$
 *
 * - **Lowpass to Highpass**:
 *   \f$ s \rightarrow \frac{\Omega_c}{s} \f$
 *
 * - **Lowpass to Bandpass**:
 *   \f$ s \rightarrow \frac{s^2 + \Omega_0^2}{B s} \f$
 *
 * - **Lowpass to Bandstop**:
 *   \f$ s \rightarrow \frac{B s}{s^2 + \Omega_0^2} \f$
 *
 * Where:
 * \f$ \Omega_0 = \sqrt{\Omega_l \cdot \Omega_h}, \quad B = \Omega_h - \Omega_l \f$
 *
 * After analog transformation, apply the bilinear transform:
 * \f[ s = \frac{2}{T} \cdot \frac{z - 1}{z + 1} \f]
 *
 * @note
 *    This implementation is designed for C++20 ``constexpr``.
 *
 * @warning
 *    High-order filters can introduce high phase delays and should be used with caution.
 *    For most applications, low-pass and high-pass filters of order 2 or lower are sufficient.
 *
 *    If results are suspicious, verify filter coefficients using external tools
 *    such as MATLAB or Python (e.g., SciPy).
 *
 *
 * @section Usage
 *
 * To use this implementation, include this header file and instantiate the
 * ``Butterworth`` class with the desired filter order, type, and parameters.
 * Then, pass those coefficients into a ``DiscreteFilter``.
 *
 * @code
 *    Coefficients coe =  butterworth<1, LOWPASS>(wc, Ts);
 *    DiscreteFilter<2> Filter(coe);
 *
 * @endcode
 *
 * @author Aiden Prevey
 * @date 4/29/2025
 * @version 2.0
 */
namespace filter {

enum FilterType : uint8_t
{
    LOWPASS  = 0b00,
    HIGHPASS = 0b01,
    BANDPASS = 0b10,
    BANDSTOP = 0b11,
};

/**
 * used to transform poles from the laplace domain to the
 * Z domain for discrete time using the bilinear transform
 *
 * @param [in] s a pole or zero from the laplace domain
 * @param [in] Ts the sample time
 * @return a complex number corresponding to the Z domain location
 */
std::complex<double> inline s2z(std::complex<double> s, double Ts)
{
    return (static_cast<std::complex<double>>(1.0) + (Ts / 2) * s) /
           (static_cast<std::complex<double>>(1.0) - (Ts / 2) * s);
}

/**
 * used to multiply out a series of zeros to obtain a list of coefficients
 *
 * @param [in] zeros a vector of complex poles or zeros to multiply out, works
 * for any polynomial in the form of (x - z1)(x - z2)...(x - zn) where z1, z2, ... zn
 * are the value of zeros.
 *
 * @return a vector of coefficients for the polynomial with the 0th index being the
 * constant term and the last index being the leading coefficient
 * @note the coefficients are returned as doubles, the imaginary part of the
 * complex number is ignored
 */
template <uint8_t ORDER>
std::array<double, ORDER + 1> expandPolynomial(std::array<std::complex<double>, ORDER> zeros)
{
    std::array<std::complex<double>, ORDER + 1> coefficients{
        std::complex<double>(1.0, 0.0)};  // Start with 1

    for (size_t i = 0; i < ORDER; i++)
    {
        // Multiply current polynomial by (x - zero)
        std::array<std::complex<double>, ORDER + 1> newCoefficients{std::complex<double>(0.0, 0.0)};
        for (size_t j = 0; j < i + 1; j++)
        {
            newCoefficients[j] -=
                coefficients[j] * zeros[i];             // Multiply current coefficient by zero
            newCoefficients[j + 1] += coefficients[j];  // Shift current coefficient
        }
        coefficients.swap(newCoefficients);
    }
    std::array<double, ORDER + 1> stripped_coefficients{};
    // Extract the real part of each coefficient, the imaginary appears to be an
    // artifact of double
    for (size_t i = 0; i < ORDER + 1; i++)
    {
        stripped_coefficients[i] = coefficients[i].real();
    }

    return stripped_coefficients;
}

/**
 * used to evaluate the frequency response of a filter at a given frequency
 * @param [in] b the numerator coefficients of the filter
 * @param [in] a the denominator coefficients of the filter
 * @param [in] w the frequency to evaluate at in rad/s
 * @param [in] Ts the sample time
 * @return the frequency response of the filter at the given frequency
 *
 */

template <uint8_t ORDER>
std::complex<double> evaluateFrequencyResponse(
    const std::array<double, ORDER + 1> &b,
    const std::array<double, ORDER + 1> &a,
    double w,
    double Ts)
{
    std::complex<double> j(0, 1);  // imaginary unit i = sqrt(-1)
    std::complex<double> numerator(0, 0);
    std::complex<double> denominator(0, 0);

    double omega = w * Ts;  // omega is in terms of rad/sample

    // Evaluate numerator: b0 + b1 * e^{-jω} + b2 * e^{-j2ω} + ...
    for (size_t k = 0; k < b.size(); ++k)
    {
        numerator += b[k] * (std::cos(omega * static_cast<double>(k)) -
                             j * std::sin(omega * static_cast<double>(k)));
    }

    // Evaluate denominator: a0 + a1 * e^{-jω} + a2 * e^{-j2ω} + ...
    for (size_t k = 0; k < a.size(); ++k)
    {
        denominator += a[k] * (std::cos(omega * static_cast<double>(k)) -
                               j * std::sin(omega * static_cast<double>(k)));
    }

    return numerator / denominator;
}
/**
 * used to calculate the magnitude of a complex number, created for constexpr useage
 * @param [in] z the complex number to calculate the magnitude of
 * @return the magnitude of the complex number
 */
double inline complexAbs(std::complex<double> z)
{
    return std::sqrt(z.real() * z.real() + z.imag() * z.imag());
}

/**
 * used to calculate the square root of a complex number, created for constexpr useage
 * @param [in] z the complex number to calculate the square root of
 * @return the square root of the complex number
 */

std::complex<double> inline complexSqrt(std::complex<double> z)
{
    double r     = std::sqrt(complexAbs(z));
    double theta = static_cast<double>(std::atan2(z.imag(), z.real())) * static_cast<double>(0.5f);
    return {r * std::cos(theta), r * std::sin(theta)};
}

constexpr uint16_t getNumCoefficients(uint8_t ORDER, FilterType type)
{
    return (1 + ((type & 0b10) != 0)) * ORDER + 1;
}

/**
 * @param[in] wc   for LOW/HIGHPASS: cutoff ωc.
 *                 for BANDPASS/BANDSTOP: lower edge ωl.
 * @param[in] Ts   sample time.
 * @param[in] type filter type, LOWPASS, HIGHPASS, BANDPASS, BANDSTOP.
 *                 defaults to LOWPASS.
 * @param[in] wh   upper edge ωh (only used for band filters).
 */
template <uint8_t ORDER, FilterType Type = LOWPASS, typename T = float>
Coefficients<getNumCoefficients(ORDER, Type), T> butterworth(double wc, double Ts, double wh = 0.0)
{
    const uint16_t COEFFICIENTS = getNumCoefficients(ORDER, Type);

    std::array<T, COEFFICIENTS> naturalResponseCoefficients;
    std::array<T, COEFFICIENTS> forcedResponseCoefficients;

    const int n = ORDER;

    // For band filters we treat wc as ωl
    double wl  = wc;
    double whp = wh;
    std::array<std::complex<double>, 2 * ORDER> bandpass_stop_poles;

    // pre-warp all edges for bilinear transform
    wl  = static_cast<double>(2.0) / Ts * std::tan(wl * (Ts / static_cast<double>(2.0)));
    whp = static_cast<double>(2.0) / Ts * std::tan(whp * (Ts / static_cast<double>(2.0)));

    // generate N prototype poles on unit circle
    std::array<std::complex<double>, COEFFICIENTS - 1> poles;
    for (int k = 0; k < n; ++k)
    {
        double theta = M_PI * (2.0 * k + 1) / (2.0 * n) + M_PI / 2.0;
        poles[k]     = std::complex<double>(std::cos(theta), std::sin(theta));
    }

    std::array<std::complex<double>, COEFFICIENTS - 1> zPoles;

    // apply the appropriate s-domaisn transform to each pole
    switch (Type)
    {
        case LOWPASS:
        {
            // poles are multiplied by wl to scale them to the desired cutoff frequency
            for (int j = 0; j < n; ++j) poles[j] = poles[j] * wl;

            // now map each analog pole into the z-plane
            for (int i = 0; i < COEFFICIENTS - 1; ++i) zPoles[i] = s2z(poles[i], Ts);
            break;
        }
        case HIGHPASS:
        {
            // applys the inverse transform of the butterworth lowpass filter
            for (int j = 0; j < n; ++j)
            {
                poles[j] = wl / poles[j];
            }
            // now map each analog pole into the z-plane
            for (int i = 0; i < COEFFICIENTS - 1; ++i) zPoles[i] = s2z(poles[i], Ts);
            break;
        }
        // In the case of a bandpass or a bandstop the amount of poles doubles.
        case BANDPASS:
        {
            /*
             *  transform in the form of s → (s² + Ω₀²) / (B · s)
             *  where:
             *      Ω₀ = √(Ω_low * Ω_high)      // Center frequency (rad/sec)
             *      B  = Ω_high - Ω_low         // Bandwidth (rad/sec)
             */
            double B    = whp - wl;
            double W0sq = whp * wl;

            for (int j = 0; j < ORDER; ++j)
            {
                std::complex<double> p = poles[j];

                std::complex<double> discriminant =
                    (p * B) * (p * B) - static_cast<std::complex<double>>(4.0) * W0sq;
                std::complex<double> root = complexSqrt(discriminant);

                bandpass_stop_poles[2 * j] =
                    (p * B + root) * static_cast<std::complex<double>>(0.5);
                bandpass_stop_poles[2 * j + 1] =
                    (p * B - root) * static_cast<std::complex<double>>(0.5);
            }

            // now map each analog pole into the z-plane
            for (int i = 0; i < COEFFICIENTS - 1; ++i) zPoles[i] = s2z(bandpass_stop_poles[i], Ts);

            break;
        }

        case BANDSTOP:
        {
            /*
             *  transform in the form of  s → B · s / (s² + Ω₀²)
             *  where:
             *      Ω₀ = √(Ω_low * Ω_high)      // Center frequency (rad/sec)
             *      B  = Ω_high - Ω_low         // Bandwidth (rad/sec)
             */
            double B    = whp - wl;
            double W0sq = whp * wl;
            for (int j = 0; j < n; ++j)
            {
                std::complex<double> p = poles[j];

                std::complex<double> discriminant =
                    B * B - (static_cast<std::complex<double>>(4.0) * -p * W0sq);
                std::complex<double> root = complexSqrt(discriminant);

                bandpass_stop_poles[2 * j] =
                    (B + root) / (static_cast<std::complex<double>>(2.0) * p);
                bandpass_stop_poles[2 * j + 1] =
                    (B - root) / (static_cast<std::complex<double>>(2.0) * p);
            }

            // now map each analog pole into the z-plane
            for (int i = 0; i < COEFFICIENTS - 1; ++i) zPoles[i] = s2z(bandpass_stop_poles[i], Ts);

            break;
        }
    }

    std::array<std::complex<double>, COEFFICIENTS - 1> zZeros;

    switch (Type)
    {
        case LOWPASS:
            // zeros: for Butterworth lowpass all z-zeros at z = –1
            zZeros.fill(std::complex<double>(-1, 0));
            break;
        case HIGHPASS:
            // zeros: for butterworth highpass all z-zeros are at z = 1
            zZeros.fill(std::complex<double>(1, 0));
            break;
        case BANDPASS:
            // zeros: for butterworth bandpass all z-zeros are at z = ±1
            for (int i = 0; i < COEFFICIENTS - 1; ++i)
            {
                zZeros[i] = (i % 2 == 0) ? std::complex<double>(1, 0) : std::complex<double>(-1, 0);
            }
            break;
        case BANDSTOP:
        {
            /* zeros: for butterworth bandstop are in a circle with radius 1 at the
             * center of the z-domain with an angle in radians per sample of the
             * center frequency. The zeros are complex conjugates of each other.
             */

            /* the notch (center) frequency in radians/sample */
            double omega0 = std::sqrt(wl * whp) * Ts;

            double realPart = std::cos(omega0);
            double imagPart = std::sin(omega0);

            std::complex<double> zeroPlus(realPart, imagPart);    // e^(+jω0)
            std::complex<double> zeroMinus(realPart, -imagPart);  // e^(-jω0)

            for (int i = 0; i < COEFFICIENTS - 1; i += 2)
            {
                zZeros[i] = zeroPlus;                                 // first of pair
                if (i + 1 < COEFFICIENTS) zZeros[i + 1] = zeroMinus;  // second of pair
            }
        }
        break;
    }

    // get expanded polynomials
    auto b = expandPolynomial<COEFFICIENTS - 1>(zZeros);
    auto a = expandPolynomial<COEFFICIENTS - 1>(zPoles);

    // scale the gain properly
    switch (Type)
    {
        case LOWPASS:
        {
            // Eval at DC
            auto freqResp = evaluateFrequencyResponse<COEFFICIENTS - 1>(b, a, 0, Ts);
            auto mag      = complexAbs(freqResp);
            double scale  = 1 / mag;
            for (auto &coef : b) coef *= scale;
            break;
        }
        case HIGHPASS:
        {
            // Eval at nyquist
            auto freqResp = evaluateFrequencyResponse<COEFFICIENTS - 1>(
                b,
                a,
                static_cast<double>(M_PI) / Ts,
                Ts);
            auto mag     = complexAbs(freqResp);
            double scale = 1 / mag;
            for (auto &coef : b) coef *= scale;
            break;
        }
        case BANDPASS:
        {
            // Eval at center frequency
            auto freqResp =
                evaluateFrequencyResponse<COEFFICIENTS - 1>(b, a, std::sqrt(wl * whp), Ts);
            auto mag     = complexAbs(freqResp);
            double scale = 1 / mag;
            for (auto &coef : b) coef *= scale;
            break;
        }
        case BANDSTOP:
        {
            // Eval at dc gain
            auto freqResp = evaluateFrequencyResponse<COEFFICIENTS - 1>(b, a, 0, Ts);
            auto mag      = complexAbs(freqResp);
            double scale  = 1 / mag;
            for (auto &coef : b) coef *= scale;
            break;
        }
    }

    // store in member arrays (reverse order)
    for (size_t i = 0; i < COEFFICIENTS; ++i)
    {
        naturalResponseCoefficients[COEFFICIENTS - i - 1] = a[i];
        forcedResponseCoefficients[COEFFICIENTS - i - 1]  = b[i];
    }

    // Store in
    Coefficients<COEFFICIENTS, T> both_coefficients;
    both_coefficients.naturalResponseCoefficients = naturalResponseCoefficients;
    both_coefficients.forcedResponseCoefficients  = forcedResponseCoefficients;
    return both_coefficients;
}
} // namespace Filter

#endif