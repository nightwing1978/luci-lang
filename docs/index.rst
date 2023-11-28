.. luci documentation master file, created by
   sphinx-quickstart on Sat Aug 19 14:47:22 2023.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to luci's documentation!
================================

.. sidebar:: About

   * :ref:`genindex`
   * :ref:`search`

Luci is an optionally typed scripting language.  It is developed in C++, has no external dependencies and is about learning what it takes to create a realistic (scripted) programming language.  It has its roots in https://interpreterbook.com/.

Luci has following:
   * A syntax that is a mix of C, C++ and Python
   * Supports imperative, object-oriented and functional style of programming
   * Supports optional typing
   * REPL

It has most features that are expected from a scripting language.  

Base Types:
   * Null 
   * Boolean: true and false
   * Integer: int that is 64-bit wide
   * Float: 64-bit floating point real numbers
   * Complex: 64-bit floating point complex numbers
   * List: [1,2,3]
   * Dictionary: { 1 : "1", 2 : "2" }
   * Sets: { 1, 2, 3 }

Control Structures:
   * Conditionals: if {} else {}
   * For-loops: for (x in y) {} 
   * While-loops: while (condition) {}

Functions:
   * Functions: let foo = fn(x) { }
   * Closures: let foo = fn(x, y) { fn(z) { x+y+z} }; let bar = foo(1,2); bar(3);
   * Anonymous functions: sorted([1,2], fn(a,b) {return a>b} )

Typing:
   * Optionally type variables: let a : double = 5.0;
   * Enforce type compatibility: let a: double = 5.0; a=1;  // fails

Types:
   * Custom type definitions: type custom { ... } 
   * Custom constructors and destructors: type custom { construct() = fn() {} }

Documentation:
   * Doc-strings for functions
   * Doc-strings at type and method level
   * Request doc-strings: print(doc(my_function));

Exceptions:
   * Raise exceptions by returning an error: if (problem) { return error("Problem", error::type::TypeError); }
   * Capture exceptions: try { return error("Problem", error::type::TypeError); }  except (d) { print("Problem found", d); }
   
Scope and lifetime control:
   * Block statement: scope { let a = 0;}

Modules:
   * Module import and scoping: import foo; foo::bar();

.. toctree::
   :maxdepth: 2
   :caption: Contents:

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
