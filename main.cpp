#include <iostream>
#include <iomanip>
#include "black_scholes.hpp"

int main() {
    double S, K, r, sigma, T;

    // current stock price
    std::cout << "Enter the current stock price (S): ";
    std::cin >> S;

    // get strike price
    std::cout << "Enter the strike price (K): ";
    std::cin >> K;

    // time to expriation
    std::cout << "Enter the time to expiration in years (T): ";
    std::cin >> T;

    // risk-free interest rate
    std::cout << "Enter the risk-free interest rate (r) [e.g., 0.05 for 5%]: ";
    std::cin >> r;

    // volatilty
    std::cout << "Enter the volatility (sigma) [e.g., 0.2 for 20%]: ";
    std::cin >> sigma;

    // option prices
    double callPrice = optionPriceCall(S, K, r, sigma, T);
    double putPrice = optionPricePut(S, K, r, sigma, T);

    // setting decimal precison
    std::cout << std::fixed << std::setprecision(2);

    // display
    std::cout << "\nOption Prices:\n";
    std::cout << "Call Price: $" << callPrice << "\n";
    std::cout << "Put Price: $" << putPrice << "\n";

    return 0;
}
