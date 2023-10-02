EPANET-RTX Library
==================

Introduction
------------
EPANET-RTX is the real-time extension to the EPANET Hydraulic Toolkit. It provides an interoperable framework for moving data between a SCADA database, various time series analysis methods, and a hydraulic solver.

This project is being developed and maintained by a multidisciplinary team with various affiliations (members listed in alphabetical order):
 
 - Ernesto Arandia-Perez (developer, University of Cincinnati)
 - Sam Hatchett (lead developer, CitiLogics)
 - Robert Janke (design team, USEPA)
 - Tom Taxon (developer, Argonne National Lab)
 - Jim Uber (design team, CitiLogics / University of Cincinnati)
 - Hyoungmin Woo (developer, University of Cincinnati)
 
Intended Audience
-----------------
 The intended audience for this software can be divided roughly into two groups:
 
 - Programmers interested in water distribution system simulation
 - Water distribution simulation engineers interested in programming
 
 The key here is an orientation towards folks who are comfortable with code. The RTX library is not a program per se, but a set of building blocks for constructing your own real-time simulation environment. If you've ever hacked EPANET or built a Matlab script for running hydraulic simulations, this library might be for you.
 
Find Out More
------------------ 
Get more info from the [official documentation](http://OpenWaterAnalytics.github.com/epanet-rtx/) and the [wiki](https://github.com/OpenWaterAnalytics/epanet-rtx/wiki).

Active Development
------------------
We are actively developing features in the following areas:
- Time Series Forecasting
- Water Age / Water Quality


Building
--------

```

conan export deps/local_export/sqlite_modern_cpp
conan export deps/local_export/epanet
conan install . --profile=x86 --build=missing -s build_type=Release
conan build . --profile=x86 --build=missing -s build_type=Release
conan export-pkg . --profile=x86 -s build_type=Release

# run tests
./build/Release/bin/rtx_test
```

if you are developing RTX as a dependent package locally, do this first:

```
conan editable add .

```

### Docker Build

```
docker buildx build -t epanetrtx-test -f epanet-rtx.docker --platform linux/amd64 .
```
