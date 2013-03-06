Coding Conventions          {#conventions}
==================
This document lists the C++ coding conventions used in the EPANET-RTX project.

Types
---------------------------
Names representing types must be in `CamelCase`, i.e. mixed case starting with upper case, such as `Point` and `TimeSeries`.

Class Methods and Functions
---------------------------
Class method and function names must be in mixed case starting with lower case, such as `id`, `flow`, `setFlow` and `headLoss`.
The term set must be used where an attribute value is set (`setFlow`) but the term get should be avoided (`flow` instead of `getFlow`).

Constants
---------------------------
Named constants (including enumeration values) must be all uppercase using underscore to separate words, such as `FLOW`, `HEAD_LOSS` and `METERS_TO_FEET`.
Floating point constant should always be written with decimal point and at least one decimal and a digit before the point, such as `0.5` and `100.0`.

Macros
---------------------------
Macros should be avoided (use const or templates). If needed, macro names must be the same of named constants plus a `RTX_` prefix, such as `RTX_MAX_C_STRING` and `RTX_DO_SOMETHING`.
Variables
Variable names must be in mixed case starting with lower case, such as index, numberOfValues and indexOfNode.
Variables with a large scope should have long names, variables with a small scope can have short names.
Variables should be declared and initialised immediately before their use.

Class Attributes
---------------------------
Private attribute names must follow the same conventions as variables, but with a single underscore as prefix, such as `_flow`, `_headLoss` and `_id`.

Reference and Pointer Declaration
---------------------------
C++ pointers and references should have their reference symbol next to the type rather than to the name, such as `int* foo;` and `char& bar;`.
Use multiples lines to declare pointers and references.

Class Declaration
---------------------------
The declaration of a class should be started with the declaration of the types used (`typedef`) followed by the declaration of constants.
Then, it should list the public methods, followed by the protected and privates methods and attributes:
    class ExampleClass
    {
        // typedef types
        // constants
        public:
        // methods
        protected:
        //methods
        // attributes
        private:
        //methods
        // attributes
    };

Containers
---------------------------
Plural form should be used on names representing a collection of objects, such as vector<double> values.
Boolean
The prefix is should be used for boolean variables and methods relating to states, such as isOpen and isClosed. The prefix "does" should be used for boolean variables and methods relating to behavior, such as doesHaveBoundaryHead.

Number of Objects
---------------------------
The postfix Count should be used for variables and methods representing a number of objects, such as NodeCount.

Files Extensions
---------------------------
C++ header files should have a .h extension. Source files should have the extension .cpp.

Class Files
---------------------------
A class should be declared in a header file and defined in a source file where the name of the files match the name of the class, such as Element.h and Element.cpp.
A file can contain more than one class if the extra classes are derived from a common parent, in which case the filename should reflect this with a family postfix (e.g., ElementFamily.h).

File Characteristics
---------------------------
Special characters like TAB and page break must be avoided.

Header File Guards
---------------------------
Header files must contain an include guard that is composed of the prefix `epanet_rtx_`, the postfix `_h` and the name of the file in lower-case, using underscore to separate words:

    #ifndef epanet_rtx_Zone_h
    #define epanet_rtx_Zone_h
    
    // file contents
    
    #endif

Comments
---------
Use single-line //, and multiline /*...*/ for comments.
Comment as much as you can and use Doxygen markup for critical sections.

Indentation and nesting
-----------------------
The indentation delimiter is two spaces (never a tab character) and should adhere to the following nesting guidelines:

    class Name {
    ...
    };
    ...
    for(...) {
        ... // use brackets even if it's a single line!
    } // ending comment if it's a long chunk of code
    ...
    void func(...) {
        ...
    } // ending comment if it's a long chunk of code
