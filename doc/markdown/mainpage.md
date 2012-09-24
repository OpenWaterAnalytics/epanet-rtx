Epanet-RTX Library                        {#mainpage}
==========

[TOC]

RTX Introduction					{#sec_intro}
============
EPANET-RTX is the real time extension to the EPANET Hydraulic Toolkit. It provides an interoperable framework for moving data between a SCADA database, various time series analysis methods, and a hydraulic solver.

This project is being developed and maintained by a multidisciplinary team with various affiliations (members listed in alphabetical order):
 
 - Ernesto Arandia-Perez (developer, University of Cincinnati)
 - Sam Hatchett (lead developer, CitiLogics)
 - Robert Janke (design team, USEPA)
 - Tom Taxon (developer, Argonne National Lab)
 - Jim Uber (design team, CitiLogics / University of Cincinnati)
 - Hyoungmin Woo (developer, University of Cincinnati)
 
Intended Audience				{#sec_audience}
=================
 The intended audience for this software can be divided roughly into two groups:
 
 - Programmers interested in water distribution system simulation
 - Water distribution simulation engineers interested in programming
 
 The key here is an orientation towards folks who are comfortable with code. The RTX library is not a program per se, but a set of building blocks for constructing your own realtime simulation environment. If you've ever hacked Epanet or built a Matlab script for running hydraulic simulations, this library might be for you.
 
How to Participate				{#sec_participate}
================== 
 This is an open-source effort, so we invite all interested and capable parties to engage. There are various ways to do this - depending on your background and interests, these avenues for interaction are available:
 
- See the [Project Website](https://github.com/OpenWaterAnalytics/epanet-rtx) and connect to the code repository.
- Use GitHub to "watch" the project -- [learn about being social](https://help.github.com/articles/be-social)
- Look over the [Coding Conventions](conventions.html)
- Use the [Issues Traker](https://github.com/OpenWaterAnalytics/epanet-rtx/issues) to:
	+ Suggest better documentation
	+ Communicate with the devs
	+ Ask questions
	+ Talk about the rtx roadmap
	+ Make a feature request
	+ File a Bug
- [Fork](https://help.github.com/articles/fork-a-repo) the project using your GitHub account
	+ Make modifications to the code, experiment, improve it
	+ Once you're happy with your modifications, document them and submit a [pull request](https://help.github.com/articles/using-pull-requests)
	+ The code maintainers can merge your changes back into the official code.
	
 
 
Getting Started					{#sec_gettingstarted}
===============
## What Is RTX? ##							{#sec_whatisrtx}
The real-time extension is a library of classes and wrappers that extend the base EPANET hydraulic & water quality simulation functionality to include data acquisition and predictive forecasting. The typical use of this toolkit would comprise building an application that would load a water utility's model and run an extended-period simulation driven by sensor measurements recorded in a scada historian. The goal of the RTX library is to make this complex task more accessible to programmers and engineers.
The user of this library may incorporate as little or much of the functionality as desired. For instance, RTX could be used only to connect to a SCADA system, clean certain data streams, and provide a predictive forecast of sensor data. Or RTX could be used just as an object abstraction layer for EPANET (e.g., for GUI development or other purposes).
 
## Prerequisites ##							{#sec_prerequisites}
Prerequisites for Epanet-RTX:
 
 - LibConfig - http://www.hyperrealm.com/libconfig/
 - GNU Scientific Library (GSL) - http://www.gnu.org/software/gsl/
 - Boost C++ Library - http://www.boost.org/
 - MySQL Connector/C++ - http://www.mysql.com/downloads/connector/cpp/
 - iODBC - http://www.iodbc.org/
 - ODBC Driver for your SCADA system, such as FreeTDS for MS-SQL providers - http://freetds.schemamania.org
 
## Concept Demonstration ##					{#sec_conceptual}
Take a look at the [demo timeseries application](demo_ts.html) - you will find a brief demonstration of some of the important time series related classes of RTX, and this could act as a testing environment to make sure your database is set up correctly.
 
You may also be interested in the [sample configuration file](sampleconfig.html) -- which is just a full explanation of the configuration options available in this version.
 
## Model Validation Example ##				{#sec_modelvalidation}
Try making the example application! The Model Validator is included here as an example of what we can do with this library -- the application first causes a simple forward simulation of a "sample town" hydraulic network to be run. The results from the simulation are stored in a MySQL database that you specify. Then the application runs a "real-time" simulation using the same model - but instead of relying on model-based controls and demand patterns, the simulation proceeds by drawing information from the prior simulation. The first simulation is the "truth" -- and acts as a synthetic SCADA historian (albeit with pervasive instrumentation). The second simulation is driven by a subset of the truth data. After running the application, the two databases can be compared as a means of validating the realtime simulation against the truth simulation.
 
 First, [have a look at the code](validation_code.html) to familiarize yourself with the conventions and names of some of the more important classes. Open up [sampletown_synthetic.cfg](validation_config_synthetic.html) -- this file is all RTX needs to load an epanet model file and connect its hydraulic states to a MySQL database. You'll need to set the database credentials for this to work, obviously, so don't forget to do that.
 
 In the example [validator application](validation_code.html), you'll notice that the executable is exceedingly short. Most of the magic is in the difference between the two configuration files. Look at [both of them together](validation_config_comparison.html) to see the differences.
 
 We hope these few examples are enough to get going on something, and act as the seed of future example applications and documentation. Please post on the project site if you have questions or would like to get involved.