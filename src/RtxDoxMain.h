/*! \mainpage EPANET-RTX
 
 \section intro_sec Introduction
 
 EPANET-RTX is the real time extension to the EPANET Hydraulic Toolkit. It provides an interoperable framework for moving data between a SCADA database, various time series analysis methods, and a hydraulic solver.
 
 This project is being developed and maintained by a multidisciplinary team with various affiliations (members listed in alphabetical order):
 
 - Ernesto Arandia-Perez (developer, University of Cincinnati)
 - Sam Hatchett (lead developer, CitiLogics)
 - Robert Janke (design team, USEPA)
 - Tom Taxon (developer, Argonne National Lab)
 - Jim Uber (design team, CitiLogics / University of Cincinnati)
 - Hyoungmin Woo (developer, University of Cincinnati)
 
 
 \section prerequisites_sec Prerequisites
 
 Prerequisites for Epanet-RTX:
 
 - LibConfig - http://freecode.com/projects/libconfigduo
 - GNU Scientific Library (GSL) - http://www.gnu.org/software/gsl/
 - Boost C++ Library - http://www.boost.org/
 - MySQL Connector/C++ - http://www.mysql.com/downloads/connector/cpp/
 - iODBC - http://www.iodbc.org/
 - ODBC Driver for your SCADA system, such as FreeTDS for MS-SQL providers - http://freetds.schemamania.org
 
 
 \section gettingstarted_sec Getting Started - Example Applications
 
 \subsection gettingstarted_tsdemo_sec Concept Demonstration
 Take a look at the \subpage demo_ts "demo timeseries application" - you will find a brief demonstration of some of the important time series related classes of RTX, and this could act as a testing environment to make sure your database is set up correctly.
 
 \subsection gettingstarted_exampleapp_sec Model Validation Example
 Try making the example application! First, \subpage validation_code "have a look at the code" to familiarize yourself with the conventions and names of some of the more important classes. Open up \subpage validation_config_synthetic "sampletown_synthetic.cfg" -- this file is all RTX needs to load an epanet model file and connect its hydraulic states to a MySQL database. You'll need to set the database credentials for this to work, obviously, so go ahead and do that.
 
 

*/

/*!
 \page demo_ts
 \section title_sec Time Series Conceptual Demonstration
 \include timeseries_demo.cpp
*/

/*!
 \page validation_code
 \section title_sec RTX Model Validation Application
 \include validator.cpp
 */

/*!
 \page validation_config_synthetic
 \section title_sec Simple Forward Simulation Configuration File
 \include sampletown_synthetic.cfg
*/








