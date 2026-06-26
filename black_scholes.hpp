// header guard, stops this file getting pulled in twice when both .cpp's include it.
// learnt the hard way that you get duplicate symbol errors without it
#ifndef BLACK_SCHOLES_HPP
#define BLACK_SCHOLES_HPP

namespace bs {

// which way the option points. a call is the right to BUY, a put is the right to SELL
enum class OptionType { Call, Put };

// all the model inputs in one struct so i'm not dragging 6 loose doubles through
// every function call. q is the continuous dividend yield, leave it at 0 if the
// underlyign doesnt pay anything.
struct Inputs {
    double S;        // spot price of the underlying right now
    double K;        // strike price
    double T;        // time to expiry in YEARS (0.5 means six months)
    double r;        // risk free rate as a decimal (0.05 = 5%)
    double sigma;    // volatility, also a decimal (0.2 = 20%)
    double q = 0.0;  // dividend yield, optional, defaults to nothing
};

// the greeks bundled up. these are the sensitivities of the option price to the
// different inputs and every trading desk watches them like a hawk. units below
// are the raw maths units, main.cpp rescales a couple of them for display.
struct Greeks {
    double delta;  // d price / d spot
    double gamma;  // d delta / d spot   (same number for a call and a put)
    double vega;   // d price / d vol    (per 1.00 of vol, i.e. per 100 vol points)
    double theta;  // d price / d time   (per year, negative means the thing decays)
    double rho;    // d price / d rate   (per 1.00 of rate)
};

// standard normal cdf and pdf. the cdf is the big one, the whole formula leans on it
double normalCDF(double x);
double normalPDF(double x);

// the two intermidiate terms. pulled out as their own functions becuase the
// greeks reuse them and i didnt want to copy paste the algebra five times
double d1(const Inputs& in);
double d2(const Inputs& in);

// price of one option of the given type
double price(const Inputs& in, OptionType type);

// little wrappers so the old call sites still read nice
double callPrice(const Inputs& in);
double putPrice(const Inputs& in);

// all five greeks at once for the given type
Greeks greeks(const Inputs& in, OptionType type);

// back out the vol that reproduces a market price. returns -1 if it cant be done,
// e.g. the quoted price sits outside the no-arbitrage bounds and theres no real answer
double impliedVol(double marketPrice, const Inputs& in, OptionType type);

// put-call parity: c - p should equal S e^-qT - K e^-rT. this hands back whats left
// over, which ought to be basically 0. handy SANITY check that nothing is broken
double parityGap(const Inputs& in);

} // namespace bs

#endif // BLACK_SCHOLES_HPP
