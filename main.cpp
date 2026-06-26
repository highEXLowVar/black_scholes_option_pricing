// command line driver for the black-scholes model. run it with no args for the old
// interactive prompts, or pass a subcommand (price / greeks / iv / curve / selftest)
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cmath>
#include "black_scholes.hpp"

using namespace bs;

namespace {

// read the five (or six) numbers that make up an Inputs, starting at argv[start].
// returns false if there arnt enough args so main can complain at the user
bool readInputs(int argc, char** argv, int start, Inputs& in) {
    if (argc < start + 5) return false;
    in.S     = std::atof(argv[start + 0]);
    in.K     = std::atof(argv[start + 1]);
    in.T     = std::atof(argv[start + 2]);
    in.r     = std::atof(argv[start + 3]);
    in.sigma = std::atof(argv[start + 4]);
    in.q     = (argc >= start + 6) ? std::atof(argv[start + 5]) : 0.0;  // q is optional
    return true;
}

void printPrices(const Inputs& in) {
    std::cout << "  call    " << std::setw(10) << callPrice(in) << "\n";
    std::cout << "  put     " << std::setw(10) << putPrice(in)  << "\n";
}

void printGreeks(const Inputs& in, OptionType type) {
    Greeks g = greeks(in, type);
    // rescale a few of them into the units traders actualy quote, see notes on the right
    std::cout << "  delta   " << std::setw(10) << g.delta          << "\n";
    std::cout << "  gamma   " << std::setw(10) << g.gamma          << "   per 1.00 move in spot\n";
    std::cout << "  vega    " << std::setw(10) << g.vega  / 100.0  << "   per 1% change in vol\n";
    std::cout << "  theta   " << std::setw(10) << g.theta / 365.0  << "   per calendar day\n";
    std::cout << "  rho     " << std::setw(10) << g.rho   / 100.0  << "   per 1% change in rate\n";
}

void runInteractive() {
    Inputs in;
    std::cout << "black-scholes option pricer\n\n";
    std::cout << "enter the current spot price (S): ";        std::cin >> in.S;
    std::cout << "enter the strike price (K): ";              std::cin >> in.K;
    std::cout << "enter time to expiry in years (T): ";       std::cin >> in.T;
    std::cout << "enter the risk free rate (r, e.g 0.05): ";  std::cin >> in.r;
    std::cout << "enter the volatility (sigma, e.g 0.2): ";   std::cin >> in.sigma;
    std::cout << "enter the dividend yield (q, 0 if none): "; std::cin >> in.q;

    std::cout << "\nprices\n";       printPrices(in);
    std::cout << "\ncall greeks\n";  printGreeks(in, OptionType::Call);
    std::cout << "\nput greeks\n";   printGreeks(in, OptionType::Put);
    std::cout << "\nput-call parity gap (should be ~0): " << parityGap(in) << "\n";
}

// dump a spot sweep as csv so the price curve can be plotted somewhere else.
// this is the exact same shape the website draws, just printed to stdout
void runCurve(const Inputs& base) {
    std::cout << "spot,call,put\n";
    double lo = 0.4 * base.K, hi = 1.6 * base.K;  // 40% to 160% of strike, decent window
    const int steps = 60;
    for (int i = 0; i <= steps; ++i) {
        Inputs in = base;
        in.S = lo + (hi - lo) * (double(i) / steps);
        std::cout << in.S << "," << callPrice(in) << "," << putPrice(in) << "\n";
    }
}

// a handful of sanity checks so i KNOW the maths still holds after i go poking at it
int runSelfTest() {
    int fails = 0;
    auto check = [&](const std::string& name, double got, double want, double tol) {
        bool ok = std::fabs(got - want) < tol;
        std::cout << (ok ? "  PASS  " : "  FAIL  ") << std::left << std::setw(26) << name
                  << "got=" << got << "  want=" << want << "\n";
        if (!ok) ++fails;
    };

    // textbook at-the-money example: S=100, K=100, T=1, r=5%, sigma=20%, no divs.
    // hull and every online calculator put the call near 10.4506 and the put near 5.5735
    Inputs a{100.0, 100.0, 1.0, 0.05, 0.20, 0.0};
    check("call at the money", callPrice(a), 10.4506, 1e-3);
    check("put at the money",  putPrice(a),   5.5735, 1e-3);
    check("parity gap is zero", parityGap(a), 0.0,    1e-9);

    // implied vol has to round trip: price it at 20%, solve, and get 20% straight back
    double mkt = callPrice(a);
    check("implied vol round trip", impliedVol(mkt, a, OptionType::Call), 0.20, 1e-4);

    // deep in the money call should be worth about its discounted intrinsic and no less
    Inputs b{150.0, 100.0, 1.0, 0.05, 0.20, 0.0};
    double floorVal = b.S - b.K * std::exp(-b.r * b.T);
    check("deep itm above intrinsic", callPrice(b) >= floorVal ? 1.0 : 0.0, 1.0, 0.5);

    std::cout << (fails == 0 ? "\nall good.\n" : "\nsome checks FAILED.\n");
    return fails;  // nonzero exit code if anything broke, nice for CI
}

void usage() {
    std::cerr <<
        "usage:\n"
        "  option_pricing                         interactive prompts\n"
        "  option_pricing price  S K T r sigma [q]   call and put price\n"
        "  option_pricing greeks S K T r sigma [q]   the five greeks for both\n"
        "  option_pricing curve  S K T r sigma [q]   csv spot sweep (spot,call,put)\n"
        "  option_pricing iv <call|put> price S K T r [q]   solve for implied vol\n"
        "  option_pricing selftest                some built in sanity checks\n";
}

} // namespace

int main(int argc, char** argv) {
    std::cout << std::fixed << std::setprecision(4);

    if (argc == 1) { runInteractive(); return 0; }  // no args, behave like the old tool

    std::string cmd = argv[1];
    Inputs in;

    if (cmd == "selftest") {
        return runSelfTest();
    }
    else if (cmd == "price") {
        if (!readInputs(argc, argv, 2, in)) { usage(); return 1; }
        printPrices(in);
    }
    else if (cmd == "greeks") {
        if (!readInputs(argc, argv, 2, in)) { usage(); return 1; }
        std::cout << "call greeks\n"; printGreeks(in, OptionType::Call);
        std::cout << "\nput greeks\n"; printGreeks(in, OptionType::Put);
    }
    else if (cmd == "curve") {
        if (!readInputs(argc, argv, 2, in)) { usage(); return 1; }
        runCurve(in);
    }
    else if (cmd == "iv") {
        // iv <call|put> MARKETPRICE S K T r [q]. sigma isnt given, its what we solve for
        if (argc < 8) { usage(); return 1; }
        OptionType type = (std::string(argv[2]) == "put") ? OptionType::Put : OptionType::Call;
        double mkt = std::atof(argv[3]);
        in.S = std::atof(argv[4]); in.K = std::atof(argv[5]);
        in.T = std::atof(argv[6]); in.r = std::atof(argv[7]);
        in.q = (argc >= 9) ? std::atof(argv[8]) : 0.0;
        in.sigma = 0.2;  // placeholder, the solver doesnt actualy trust this
        double iv = impliedVol(mkt, in, type);
        if (iv < 0.0) std::cout << "couldnt solve, that price is outside the no-arb bounds\n";
        else          std::cout << "implied vol = " << iv << "  (" << iv * 100.0 << "%)\n";
    }
    else {
        std::cerr << "unknown command '" << cmd << "'\n";
        usage();
        return 1;
    }
    return 0;
}
