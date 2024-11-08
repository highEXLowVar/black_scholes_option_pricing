#define _USE_MATH_DEFINES
#include <cmath>
#include "black_scholes.hpp"

const double PI = 3.14159265358979323846;

// functon to compute cumualtive distribution function
double normalCDF(double x) {
    return 0.5 * erfc(-x * M_SQRT1_2);
}

// calculats d1 for black sholes formula
double calculateD1(double S, double K, double r, double sigma, double T) {
    return (log(S / K) + (r + 0.5 * sigma * sigma) * T) / 
           (sigma * sqrt(T));
}

// computes d2 value
double calculateD2(double d1, double sigma, double T) {
    return d1 - sigma * sqrt(T);
}

// computes call option price
double optionPriceCall(double S, double K, double r, double sigma, double T) {
    double d1 = calculateD1(S, K, r, sigma, T);
    double d2 = calculateD2(d1, sigma, T);
    // returs call price
    return S * normalCDF(d1) - K * exp(-r * T) * normalCDF(d2);
}

// calculates put option price
double optionPricePut(double S, double K, double r, double sigma, double T) {
    double d1 = calculateD1(S, K, r, sigma, T);
    double d2 = calculateD2(d1, sigma, T);
    // returns put price
    return K * exp(-r * T) * normalCDF(-d2) - S * normalCDF(-d1);
}
