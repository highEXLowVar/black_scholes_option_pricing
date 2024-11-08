// thsi to avoid multiple inclusions of  header file
#ifndef BLACK_SCHOLES_HPP
#define BLACK_SCHOLES_HPP

double normalCDF(double x);
double calculateD1(double S, double K, double r, double sigma, double T);
double calculateD2(double d1, double sigma, double T);
double optionPriceCall(double S, double K, double r, double sigma, double T);
double optionPricePut(double S, double K, double r, double sigma, double T);

#endif // BLACK_SCHOLES_HPP
