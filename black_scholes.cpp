// windows needs this defined BEFORE cmath or the M_ constants go missing on me
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include "black_scholes.hpp"

namespace bs {

// 1/sqrt(2*pi), precomputed becuase the pdf gets hammered inside the implied vol loop
static const double INV_SQRT_2PI = 0.39894228040143267794;

double normalCDF(double x) {
    // erfc is numericaly stable way out in the tails, much nicer than a hand rolled
    // taylor series that loses all its precision once x gets big
    return 0.5 * std::erfc(-x * M_SQRT1_2);
}

double normalPDF(double x) {
    return INV_SQRT_2PI * std::exp(-0.5 * x * x);
}

// sigma * sqrt(T) shows up everywhere so it gets its own helper
static double sigmaRootT(const Inputs& in) {
    return in.sigma * std::sqrt(in.T);
}

double d1(const Inputs& in) {
    // the famous d1. log moneyness plus the drift, all scaled by the vol over the period
    return (std::log(in.S / in.K) + (in.r - in.q + 0.5 * in.sigma * in.sigma) * in.T)
           / sigmaRootT(in);
}

double d2(const Inputs& in) {
    return d1(in) - sigmaRootT(in);
}

double price(const Inputs& in, OptionType type) {
    // degenerate corner: at or past expiry, or with no vol at all, the formula wants to
    // divide by ~0. so we skip it and just hand back the discounted intrinsic value.
    // with zero vol the underlying is deterministic, it grows at (r - q), so the payoff
    // is known and we only have to discount it
    if (in.T <= 0.0 || in.sigma <= 0.0) {
        double sDisc = in.S * std::exp(-in.q * in.T);
        double kDisc = in.K * std::exp(-in.r * in.T);
        if (type == OptionType::Call) return std::max(sDisc - kDisc, 0.0);
        return std::max(kDisc - sDisc, 0.0);
    }

    double D1 = d1(in);
    double D2 = D1 - sigmaRootT(in);
    double sDisc = in.S * std::exp(-in.q * in.T);  // spot, carried back for dividends
    double kDisc = in.K * std::exp(-in.r * in.T);  // strike, discounted at the rate

    if (type == OptionType::Call)
        return sDisc * normalCDF(D1) - kDisc * normalCDF(D2);
    // put. could just use parity here but writing it out keeps the two paths symetric
    return kDisc * normalCDF(-D2) - sDisc * normalCDF(-D1);
}

double callPrice(const Inputs& in) { return price(in, OptionType::Call); }
double putPrice(const Inputs& in)  { return price(in, OptionType::Put); }

Greeks greeks(const Inputs& in, OptionType type) {
    Greeks g{};

    // same corner case as price(). right at expiry the greeks blow up to infinity or
    // collapse to a step, so we just report the limiting delta and zero the rest rather
    // then letting NaNs leak out
    if (in.T <= 0.0 || in.sigma <= 0.0) {
        if (type == OptionType::Call) g.delta = (in.S > in.K) ? 1.0 : 0.0;
        else                          g.delta = (in.S < in.K) ? -1.0 : 0.0;
        return g;  // gamma/vega/theta/rho all left at 0
    }

    double D1 = d1(in);
    double D2 = D1 - sigmaRootT(in);
    double sqrtT = std::sqrt(in.T);
    double eqt = std::exp(-in.q * in.T);  // dividend carry factor
    double ert = std::exp(-in.r * in.T);  // discount factor
    double pdf = normalPDF(D1);

    // gamma and vega dont care which way the option points, theyre the SAME for both.
    // took me a second to belive that the first time i saw it
    g.gamma = eqt * pdf / (in.S * in.sigma * sqrtT);
    g.vega  = in.S * eqt * pdf * sqrtT;

    if (type == OptionType::Call) {
        g.delta = eqt * normalCDF(D1);
        g.theta = -(in.S * eqt * pdf * in.sigma) / (2.0 * sqrtT)
                  - in.r * in.K * ert * normalCDF(D2)
                  + in.q * in.S * eqt * normalCDF(D1);
        g.rho   = in.K * in.T * ert * normalCDF(D2);
    } else {
        g.delta = -eqt * normalCDF(-D1);
        g.theta = -(in.S * eqt * pdf * in.sigma) / (2.0 * sqrtT)
                  + in.r * in.K * ert * normalCDF(-D2)
                  - in.q * in.S * eqt * normalCDF(-D1);
        g.rho   = -in.K * in.T * ert * normalCDF(-D2);
    }
    return g;
}

double impliedVol(double marketPrice, const Inputs& in, OptionType type) {
    // first, refuse the impossible. an option price has to sit between its intrinsic
    // value and the value of the underlying itself, otherwise theres free money lying
    // around and no vol can ever reproduce the quote
    double sDisc = in.S * std::exp(-in.q * in.T);
    double kDisc = in.K * std::exp(-in.r * in.T);
    double lowBound  = (type == OptionType::Call) ? std::max(sDisc - kDisc, 0.0)
                                                  : std::max(kDisc - sDisc, 0.0);
    double highBound = (type == OptionType::Call) ? sDisc : kDisc;
    if (marketPrice < lowBound - 1e-9 || marketPrice > highBound + 1e-9)
        return -1.0;  // outside the bounds, bail instead of looping forever

    Inputs guess = in;
    guess.sigma = 0.2;  // 20% is a reasonable starting guess for most names

    // newton-raphson first. price is smooth in vol and vega is its derivative, so this
    // converges QUADRATICALLY when it behaves
    for (int i = 0; i < 100; ++i) {
        double diff = price(guess, type) - marketPrice;
        if (std::fabs(diff) < 1e-8) return guess.sigma;
        double v = greeks(guess, type).vega;  // d price / d sigma
        if (v < 1e-12) break;                 // vega died on us, drop to bisection
        guess.sigma -= diff / v;
        if (guess.sigma <= 0.0) guess.sigma = 1e-4;  // dont let it wander negative
    }

    // fallback: dumb but bulletproof bisection. price is monotone increasing in vol so a
    // bracket ALWAYS contains the answer, this just cant fail to converge
    double lo = 1e-6, hi = 5.0;  // 500% vol is a stupidly wide net, nothing escapes it
    for (int i = 0; i < 200; ++i) {
        double mid = 0.5 * (lo + hi);
        guess.sigma = mid;
        double diff = price(guess, type) - marketPrice;
        if (std::fabs(diff) < 1e-8) return mid;
        if (diff > 0.0) hi = mid; else lo = mid;
    }
    return 0.5 * (lo + hi);
}

double parityGap(const Inputs& in) {
    double c = price(in, OptionType::Call);
    double p = price(in, OptionType::Put);
    double rhs = in.S * std::exp(-in.q * in.T) - in.K * std::exp(-in.r * in.T);
    return (c - p) - rhs;  // wants to be ~0
}

} // namespace bs
