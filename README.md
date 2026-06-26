# Black-Scholes Option Pricer

A small C++ library and command line tool that prices European options with the Black-Scholes model. I first wrote this a while back just to knock the rust off my C++, and I came back to it to turn it into something I would actually want to show people. It now does the full set of Greeks, solves for implied volatility, handles dividends, and checks its own answers.

Everything is plain standard C++17. No external libraries, no build system you have to install first, nothing to download. Two source files and a header.

## What it does

The model takes the usual five inputs (spot, strike, time to expiry, the risk free rate and volatility) plus an optional dividend yield, and gives you back:

- **Call and put prices** from the closed form Black-Scholes formula.
- **The five Greeks** for both calls and puts: delta, gamma, vega, theta and rho. These are the sensitivities of the price to each input, and they are what you actually hedge with. The CLI prints them in the units traders quote (vega per 1% of vol, theta per day, rho per 1% of rate).
- **Implied volatility**, which is the reverse problem. You give it a market price and it finds the volatility that reproduces that price. It uses Newton-Raphson, which is fast because vega is exactly the derivative it needs, and falls back to bisection if Newton wanders off. There is a no-arbitrage bounds check up front so it refuses impossible prices instead of spinning forever.
- **A put-call parity check** so you can confirm the call and put prices are internally consistent.

## Why it is better than the first version

The first version was about forty lines and priced a single call and a single put from hardcoded prompts. That was the whole thing. This version:

- splits the maths into a reusable library (`black_scholes.hpp` / `black_scholes.cpp`) instead of leaving it tangled up with the input/output code, so you can pull the pricer into another program without dragging `main` along with it,
- adds the Greeks and implied volatility, which is most of the reason anyone opens this model in the first place,
- handles dividends through a continuous yield,
- deals with the awkward edge cases (zero time left, zero volatility) by returning the discounted intrinsic value instead of dividing by zero and spitting out NaN,
- uses `erfc` for the normal CDF, which stays accurate in the tails where a naive series falls apart,
- ships a `selftest` command that checks the prices against known textbook values, confirms parity holds, and round trips implied vol back to the input. So if I break something while tinkering, the program tells me.

## Building

You need any C++17 compiler. With make:

```
make          # builds ./option_pricing
make test     # builds, then runs the self checks
```

Or just call the compiler yourself:

```
g++ -std=c++17 -O2 main.cpp black_scholes.cpp -o option_pricing
```

On Windows with MSYS2 / MinGW the same `g++` line works, you just get `option_pricing.exe`.

## Using it

Run it with no arguments and it walks you through the inputs like the old one did:

```
$ ./option_pricing
black-scholes option pricer

enter the current spot price (S): 100
enter the strike price (K): 100
...
```

Or skip the prompts with a subcommand:

```
$ ./option_pricing price 100 100 1 0.05 0.2
  call       10.4506
  put         5.5735

$ ./option_pricing greeks 100 100 1 0.05 0.2
call greeks
  delta       0.6368
  gamma       0.0188   per 1.00 move in spot
  vega        0.3752   per 1% change in vol
  theta      -0.0176   per calendar day
  rho         0.5323   per 1% change in rate
...

$ ./option_pricing iv call 10.4506 100 100 1 0.05
implied vol = 0.2000  (20.0000%)
```

That last one is the round trip: the price 10.4506 is exactly what 20% vol produces above, so the solver hands 20% straight back. Feed it a higher market price and you get a higher implied vol, which is the whole point. The arguments are always in the order `S K T r sigma` with the dividend yield `q` as an optional sixth. Time is in years and the rate and vol are decimals, so 5% is `0.05`.

There is also a `curve` command that prints a CSV of the call and put price across a sweep of spot prices. That is the same shape as the chart on my site, the model just prints it instead of drawing it:

```
$ ./option_pricing curve 100 100 1 0.05 0.2 > prices.csv
```

## A note on the model itself

Black-Scholes assumes the stock follows a geometric Brownian motion with constant volatility, that you can trade continuously with no costs, and that the option is European so it only pays out at expiry. None of that is exactly true in real markets, which is the whole reason implied volatility is not flat across strikes (the famous volatility smile). I am treating this as a clean, well understood baseline to implement carefully rather than a claim about how options really trade. The interesting follow up is where the assumptions break, and that is the direction I would take this next.

## Files

```
black_scholes.hpp    the public API: Inputs, Greeks, and the function declarations
black_scholes.cpp    the actual maths: CDF, d1/d2, prices, Greeks, implied vol
main.cpp             the command line driver and the self tests
Makefile             build, test, clean
```
