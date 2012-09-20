Epanet-RTX
==========

Introduction
------------
EPANET-RTX is the real time extension to the EPANET Hydraulic Toolkit. It provides an interoperable framework for moving data between a SCADA database, various time series analysis methods, and a hydraulic solver.

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
 
 The key here is an orientation towards folks who are comfortable with code. The RTX library is not a program per se, but a set of building blocks for constructing your own realtime simulation environment. If you've ever hacked Epanet or built a Matlab script for running hydraulic simulations, this library might be for you.
 
How to Participate
------------------ 
 This is an open-source effort, so we invite all interested and capable parties to engage. There are various ways to do this - depending on your background and interests, these avenues for interaction are available:
 
- See the [Project Website](https://github.com/samhatchett/epanet-rtx) and connect to the code repository.
- Use the [Issues Traker](https://github.com/samhatchett/epanet-rtx/issues) to:
	+ Suggest better documentation
	+ Communicate with the devs
	+ Make a feature request
	+ File a Bug
 

What Is RTX?
------------
The real-time extension is a library of classes and wrappers that extend the base EPANET hydraulic & water quality simulation functionality to include data aquisition and predictive forecasting. The typical use of this toolkit would comprise building an application that would load a water utility's model and run an extended-period simulation driven by sensor measurements recorded in a scada historian. The goal of the RTX library is to make this complex task more accessible to programmers and engineers.
The user of this library may incorporate as little or much of the functionality as desired. For instance, RTX could be used only to connect to a SCADA system, clean certain data streams, and provide a predictive forecast of sensor data. Or RTX could be used just as an object abstraction layer for EPANET (e.g., for GUI developement or other purposes).
 
Prerequisites
-------------
Prerequisites for Epanet-RTX:
 
 - LibConfig - http://www.hyperrealm.com/libconfig/
 - GNU Scientific Library (GSL) - http://www.gnu.org/software/gsl/
 - Boost C++ Library - http://www.boost.org/
 - MySQL Connector/C++ - http://www.mysql.com/downloads/connector/cpp/
 - iODBC - http://www.iodbc.org/
 - ODBC Driver for your SCADA system, such as FreeTDS for MS-SQL providers - http://freetds.schemamania.org