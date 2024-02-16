Built-in functions and modules
==============================

1. Functions
------------

* ``type_str``: fn(all) -> str : return the type of an object in a string
* ``error``: fn(str, int) -> error : create an error with given message and number indicating its type

* ``clone``: fn(any) -> any: returns a clone of the given object
* ``doc``: fn(any) -> str: returns the doc string for a given object

* ``print``: fn(any) -> str: returns the doc string for a given object
* ``eprint``: fn(any) -> str: returns the doc string for a given object
* ``input_line``: fn(str) -> str: returns the string from the standard input device
* ``version``: fn() -> [int]: returns the version as an array of 3 integers, [major, minor, patch]
* ``arg``: fn() -> [str]: returns the command line arguments as an array of string
* ``format``: fn(str, all) -> str: format a string

* ``run``: fn(str) -> null: run the script in the given filename 
* ``run_once``: fn(str) -> null: run the script in the given filename once over the lifetime of the overall program
* ``exit``: fn(int) -> null: exit the program/interpreter with given error code

* ``scope_names``: fn() -> [str]: returns the list of names of identifiers visible in the current scope

* ``array``: fn() -> [all]: create an array
* ``array_double``: fn() -> [double]: create an array of doubles
* ``array_complex``: fn() -> [complex]: create an array of complex
* ``complex``: fn(double, double) -> complex: create a complex number
* ``dict``: fn() -> {all:all}: create an empty dictionary 
* ``set``: fn() -> {all}: create an empty set
* ``range``: fn(int) -> range: create a range with given upper bound
* ``range``: fn(int, int) -> range: create a range with given lower and upper bound
* ``range``: fn(int, int, int) -> range: create a range with given lower, upper bound and stride

1.1 Array functions
~~~~~~~~~~~~~~~~~~~

* ``append``: fn([all], all) -> [all]: append an object to an array
* ``slice``: fn([all], int,int ) -> [all]: return a slice of an array
* ``slice``: fn([all], range ) -> [all]: return a slice of an array defined by a range
* ``update``: fn([all], int, all ) -> [all]: update an element in the array with a new object
* ``rotate``: fn([all], int ) -> [all]: rotate an array with given amount
* ``reverse``: fn([all]) -> [all]: reverse an array, returns a reference to self
* ``sort``: fn([all]) -> bool: sort an array, returns true when sorting was succesful
* ``sort``: fn([all], fn(all,all) -> bool) -> bool: sort an array given provided comparison function
* ``reversed``: fn([all]) -> [all]: reverse a copy of the given array but reversed
* ``rotated``: fn([all], int) -> [all]: rotate a copy of the given array by given amount
* ``sorted``: fn([all]) -> [all]: provide a copy of the given array but sorted
* ``sorted``: fn([all], fn(all,all) -> bool) -> [all]: return a copy of the array sorted by provided comparison function
* ``is_sorted``: fn([all]) -> bool: returns true when given array is sorted
* ``is_sorted``: fn([all], fn(all,all) -> bool) -> bool: returns true when the given array is sorted by provided comparison function

1.2 Dictionary functions
~~~~~~~~~~~~~~~~~~~~~~~~

* ``values``: fn({all:all}) -> [all]: returns the values of a given dictionary
* ``keys``: fn({all:all}) -> [all]: return the keys of a given dictionary

1.3 String conversion
~~~~~~~~~~~~~~~~~~~~~

* ``to_bool``: fn(str) -> bool: interprets the given str and returns the boolean value
* ``to_int``: fn(str) -> int: interpret the given str to an int
* ``to_double``: fn(str) -> double: interpret the given str to a double

1.3 I/O functions
~~~~~~~~~~~~~~~~~

* ``open``: fn(str, str) -> io: open the given filename with given mode and return the IO object


2. Module overview
------------------

* error_type: working with error types
* json: serializing and deserializing to json
* math: mathematical functions
* os: communication with the OS and file system
* regex: regular expression
* time: working with time
* threading: threading support
* typing: working with types

3. error_type
-------------

Constants:
* ``undefined_error``: int : unknown error
* ``type_error``: int : error against the type system
* ``const_error``: int : error in const correctness
* ``identifier_not_found``: int : error when trying to find an identifier
* ``identifier_already_exists``: int : error when trying to define an identifier that is already existant within scope
* ``value_error``: int : error when trying to produce a value
* ``key_error``: int : error when assuming a key exist within a set or dictionary and it does not
* ``index_error``: int : error when trying to index beyond the allowed ranges
* ``import_error``: int : error when trying to import a module
* ``syntax_error``: int : error when trying to parse a fragment of source
* ``os_error``: int : error when trying to open a file or any OS related errors

Example:

.. code-block:: 

    import error_type;
    try 
    {
        let a : int = 3.0;
    }
    except ( e )
    {
        if (e.error_type() == error_type.type_error)  
        {
            print("type error on assignment");
        }
        else
        {
            print("unknown error");
        }
    }


4. json
-------

* ``load``: fn(str) -> all : take a JSON string and convert it into an object
* ``dump``: fn(all) -> str : take an object and dump it into a JSON string 

Example:

.. code-block:: 

    let d : [all] = [1, 2.0, "three"];
    let s = json.load(json.dump(d));
    print("s =", s);
    
    let e : {str:all} = {
        "a" : 1,
        "b" : 2.0,
        "c" : "three",
    };
    let t = json.load(json.dump(e));

    print("e =", e);
    print("t =", t);


5. math
-------

* ``abs``: fn(double) -> double : take absolute value of double
* ``acos``: fn(double) -> double: arc cosine of double
* ``asin``: fn(double) -> double: arc sine of double
* ``atan``: fn(double) -> double: arc tangent of double
* ``cbrt``: fn(double) -> double: cube root of double
* ``cos``: fn(double) -> double: cosine of double
* ``erfc``: fn(double) -> double: complementary error function of double
* ``exp``: fn(double) -> double: e raised to power of given double
* ``lgamma``: fn(double) -> double: natural logarithm of the gamma function
* ``log``: fn(double) -> double: logarithm of a double
* ``log10``: fn(double) -> double: logarithm of a double with base 10
* ``round``: fn(double) -> double: round to nearest integer
* ``sin``: fn(double) -> double: sine of a double
* ``sqrt``: fn(double) -> double: square root of a double
* ``tan``: fn(double) -> double: tangent of a double
* ``tgamma``: fn(double) -> double: gamma of a double
* ``trunc``: fn(double) -> double: truncated value of a double
* ``pow``: fn(double, double) -> double: raise a double by a power given by another double

6. os
-----

* ``absolute``: fn(str) -> str: convert a path defined in a string into an absolute path
* ``canonical``: fn(str) -> str: convert a path defined in a string into a canonical path
* ``weakly_canonical``: fn(str) -> str: convert a path defined in a string into a weakly canonical path

* ``current_path``: fn() -> str: return the current path
* ``temp_directory_path``: fn() -> str: return a path to a temporary directory
* ``exists``: fn(str) -> bool: returns true when a given path exists
* ``create_directory``: fn(str) -> bool: create the provided directory, true when succesful
* ``create_directories``: fn(str) -> bool: create the provided hierarchical directory path, true when succesful

* ``remove``: fn(str) -> bool: remove the provided path, true when succesful
* ``remove_all``: fn(str) -> bool: remove the provided path including subdirectories, true when succesful

* ``copy``: fn(str, str) -> null: copy the given file from source to destination
* ``rename``: fn(str, str) -> null: rename the given file from source to destination

* ``list_dir``: fn(str) -> [str]: list the content of the given directory name and return the list of encountered files
* ``list_dir_recursively``: fn(str) -> [str]: list the content of the given directory name, iterate through subdirectories and return the list of encountered files

* ``get_env``: fn(str) -> str: returns the value of the given environment variable
* ``system``: fn(str) -> int: execute the provided command as a system call and return the return code of the process after execution

Example:

.. code::code-block

    import os;
    print("PATH=", os::getenv("PATH"));
    print("Executing echo=", os::system("echo boo"));
    print("Directory contents\n", os::list_dir_recursively(os::path::join(["."])));


6.1 os::path
~~~~~~~~~~~~

* ``join``: fn([str]) -> str: join an array of path elements into a single path
* ``root_name``: fn(str) -> str: 
* ``root_directory``: fn(str) -> str: 
* ``root_path``: fn(str) -> str: 
* ``relative_path``: fn(str) -> str: 
* ``parent_path``: fn(str) -> str: 
* ``filename``: fn(str) -> str: 
* ``stem``: fn(str) -> str: 
* ``extension``: fn(str) -> str: 
* ``is_relative``: fn(str) -> bool: 
* ``is_absolute``: fn(str) -> bool: 

Example using the os::path functionality:

.. code:: 

    import os;
    import os::path;

    print("Current working directory:", os::current_path());


7. regex
--------

Functions:

* ``regex``: fn() -> regex: create a regex object
* ``match``: fn(regex, str) -> <null, [str]>: return the matches of the regex given the string 
* ``search``: fn(regex, str) -> <null, [str]>: return the found values of the regex given the string
* ``replace``: fn(regex, str, str) -> str: replace all the occurences in the string provided by the other string using the regex for pattern matching

Constants:

* ``icase``: int
* ``nosubs``: int
* ``optimize``: int
* ``collate``: int
* ``ECMAscript``: int
* ``basic``: int
* ``extended``: int
* ``awk``: int
* ``grep``: int
* ``egrep``: int

Example:

.. code::

    import regex;
    let re = regex::regex;

    print( regex::match(re("[a-z]+\\.txt"), "foo.txt") );
    print( regex::match(re("[a-z]+\\.txt"), "bar.dat") );

    print( regex::replace(re("a|e|i|o|u"), "Quick brown fox", "[$&]"), "Q[u][i]ck br[o]wn f[o]x" );

8. time
-------

Functions:

* ``time``: fn() -> double: gives the time since the epoch (system dependent).

Example:

.. code::
    
    import time
    import threading

    let start_time : double = time.time();
    threading.sleep(2.0);
    let time_elapsed : double = time.time() - start_time;
    print("Time elapsed= ", time_elapsed);


9. threading
------------

* ``thread``: fn( fn() -> all ) -> thread: create a thread object and execute the specified function in it
* ``sleep``: fn(double) -> null: sleep the current thread for the given amount of seconds

Example:

.. code::

    import threading;

    let f = fn(b : int) {
    let val = b;
        for (i in [0,1,2,3,4]) 
        {
            val = val + i;
            threading::sleep(0.01);
        }
        return val;
    };

    let threads = [];
    for (i in range(4)) 
    {
        threads.push_back(threading::thread(f,i));
    }

    for (thread in threads) {
        thread.start();
    }

    let sum = 0;
    for (thread in threads) {
        thread.join();
        sum += thread.value();
    }

    print("Sum from all threads=", sum);

10. typing
----------

* ``is_compatible_type_str``: fn(str, str) -> bool: compares two type strings and returns true when the first is compatible with the second one

Example:

.. code::

    import typing;
    print("Types are compatible", typing::is_compatible_type_str("double", "<double,int>"));

