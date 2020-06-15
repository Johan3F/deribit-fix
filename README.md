# Deribit

Having a connection to the Deribit exchange using quickfix and c++

# Installing 

The requirements for this project are:
- quicfix: http://www.quickfixengine.org/
- boost: https://www.boost.org/
- CMake: https://cmake.org/

# What it's doing?

The idea is to have a common ground for different strategies to be able to work at the same time. Each one of them will be running their own quickfix engine (althought quickfix allows multiple session within itself). Each one should be running in their own account (portfolio like structure that Deribit offers.

Strategies:
- test strategy: This is just for testing all connectivity with the test environment with Deribit
- gamma scalper: Work in progress. Gamma scalper that uses a stradle and hedges deltas in the underlying future