Language
========

1. Hello World
--------------

.. code:: 

    print("Hello World!");


``print`` will display on the standard output.  To display information on the error output, use ``eprint``.

Requesting input from a user on standard-in is done through ``input_line``.

.. code:: 

    let user_input = input_line("Prompt:");
    print("User provided:", user_input);


2. Basic types
--------------

2.1 Numbers
~~~~~~~~~~~

Luci supports a number of basic types, such as numbers.  Both integers as well as floats.  Both are 64-bit wide.

.. code:: 

    let a = 3 * 5;
    let b = 3.0 * 5.0;
    
There is no support for mixed type operations.  Adding an integer to a float will result in an error.  To work with mixed types an explicit conversion is required:

.. code:: 

    let a = 3;
    let b = to_double(a) * 5.0;

complex numbers are native types, they are constructed by using the function `complex`:

.. code:: 

    let c = complex(1.0,2.0);

2.2 Strings
~~~~~~~~~~~

In Luci text is stored in strings.  Strings are of the type `str` (more on types later).  Adressing parts of a string will again yield a string, even if the outcome is the equivalent of one character.

.. code:: 

    let t = "text";     // a string
    let c = t[0];       // again a string

Luci does not support any encoding schemes at this moment. Slicing is also support using a range object.

.. code:: 

    let t = "text";         // a string
    let s = t[0..2];        // a string of the first two characters


Strings have a number of functions:

* ``clear``: clears a string and returns itself
* ``is_empty``: return a boolean if the string is empty or not
* ``size``: returns the length of the string
* ``starts_with``: return a boolean if a string starts with another given string
* ``ends_with``: return a boolean if a string ends with another given string
* ``find``: returns the index of the given string, returns -1 when not found
* ``replace``: replaces all occurences of a given string with another one
* ``split``: split a given string into a list of strings using any of given separators, the list of separators is also passed in as string
* ``join``: join a given list of strings by using the string and returning the joined string

More advanced string manipulation can be found in the ``regex`` module.

3. Collections and ranges
-------------------------

There are three main types of collections: a list, dictionary and set.  

.. code:: 

    let l = [0,1,2,3.0,"bla"];      // a list can contain any values
    let d = {0 : "a", 1 : "b"};     // a dictionary maps a key to a value
    let s = {0,1,2,3,4};

A list can contain values of any kind.  Internally a list of doubles and complex numbers is treated differently and more efficiently when possible.

Items used as a key in a dictionary or set needs to be so called `hashable`.  The basic types like a boolean, integer, float and strings are all hashable 
types.  Compound types like a list, set or dictionary are only `hashable` when they are in `frozen` state and immutable.

3.1 Range
~~~~~~~~~

A range is a special type that represents a range of integer numbers with an optional stride.  Ranges can be constructed with the built-in ``range`` function, but there is also a dedicated syntax.

.. code:: 

    let r = range(0,5);     // a range covering 0,1,2,3,4
    let s = 0..5;           // a range covering 0,1,2,3,4
    let t = 0..5:2;         // range with stride 2 covering 0,2,4

Ranges can be used to index into arrays and strings.

4. Control Structures
---------------------

Control structures that are supported are if-then-else, for-loop and while-loops.  The statements that they control all need to be scoped using `{`.

.. code:: 

    if (true) 
    {
        print("Value was true");
    }
    else
    {
        print("Value was false");
    }

While loops are executed while the condition remains true.

.. code:: 

    let a = 5;
    while (a>0)
    {
        print("Value of a=",a);
        a -= 1;
    }

For loops always need an iterable object.  Iterable objects are arrays, strings, sets, ranges and dictionaries.

.. code:: 

    >> for (c in "bla") { print(c) }
    b
    l
    a

The `break` statement allows to break out of the scope where it is used, both for while and for loops.  The `continue` statement will jump
forward to the end of the scope.  

5. Functions
------------

Functions are objects like any other and are defined in a similar way.  The difference is that they have the `fn` keyword to
indicate a function definition follows.

.. code:: 

    let fibonacci = fn( x ) 
    {
        if (x <= 1 ) 
        {
            return x;
        }
        return fibonacci(x-1)+fibonacci(x-2)
    }

    print("fibonacci(5)=", fibonacci(5));

After the `fn` the list of formal parameters follows, followed by the function body.  `return` can be used to return from the given function.  
The return value of the function is either the value given by the `return` statement or the last evaluated statement.  In example below
the function `give_five` returns 5.

.. code:: 

    let give_five = fn( ) { 5; }

Functions are allow to be recursive as shown in `fibonacci` and capture their environment, becoming a closure.

.. code:: 

    let f = fn(x) 
    {
        let g = fn(y) 
        {
            return x + y;
        }
        return g;
    }
    print(f(1)(5));


6. Typing
---------

Typing is optional in Luci.  When no type is explicitly declared, then the type is `all` which matches to any other type.  
This also means the type can change during its lifetime.

.. code:: 

    let a = 5.0;
    a = "now a string";

and 

.. code:: 

    let a : all = 5.0;
    a = "now a string";

are equivalent from a typing perspective.  There is also another special type `any` which also matches any other specific type, 
but then pins the type to the variable declaration.  `any` is convenient to avoid the need to be specific on the type but 
signal that the type cannot change during the lifetime of the variable.

.. code:: 

    let a : any = 5.0;
    a = "now a string";     // this will produce a TypeError 


6.1 Specifying a type
~~~~~~~~~~~~~~~~~~~~~

A type is specified by declaring it during a let variable statement, a formal parameter or a return value.

.. code:: 

    let a : double = 5.0;
    let f = fn( a : int ) -> int
    {
        return a + 2;
    };
    let g : fn(int) -> int = fn( a : int ) -> int 
    {
        return a + 3;
    };

The stringified value of a type of an object can be requested using the `type_str`.

.. code:: 

    let a : double = 5.0;
    print(type_str(a));     // will print "double"

6.1 Compound types
~~~~~~~~~~~~~~~~~~

The type system allows for defining compound types, such as those of an array, dictionary or set.  

.. code:: 

    let a : [] = [1, "a", 5.0];
    let b : [double] = [1.0, 5.0];
    let c : [<double, int>] = [1, 5.0];

The `<>` construct is used to enumerate the possible choices of types that are accepted.  An example for dictionary type definition is below:

.. code:: 

    let d : { int : <double, str>} = { 0 : "zero", 1 : 1.0 };

7. Exception Handling
---------------------

Luci supports exception handling.  Exception are `error` being returned from a function or program.  If the `error` is not catched it will 
continue to propagate until it does or the program ends.  There is no special syntax to raise or throw them, creating the error or returning 
it from a function is enough.

.. code::  

    import error_type;
    try 
    {
        let a : int = 3.0;
    } 
    except (d)
    {
        print(d.error_type(), error_type.type_error, "type error on assignment");
    };

Exception can be nested.  Requesting more information of what happened can be done using the built-in methods on the `error` type:

    * message(): return the message part of the exception/error
    * error_type(): an integer representing the error type
    * file_name(): the filename where the error occured originally occured
    * line(): the line in the filename where the error occured originally occured
    * column(): the column in the filename where the error occured originally occured

8. Custom types
---------------

New types can be defined, a type can have properties and methods.  There are two special methods `construct` and `destruct` that are 
called respectively at the begin and end of the lifetime of the ojects created in this type.

.. code:: 

    type custom 
    {
        a : int = 1;
        const b : int = 3;

        construct = fn() -> null { this.a = 5; return null;};
        destruct = fn() -> null { return null; };
        get_a = fn() -> int { return this.a; };
        get_a_plus_1 = fn() -> int { return 1 + this.a; };
    };

    let c : custom = custom();
    print(custom.get_a());
    print(custom.a);

Within a method the object can be referenced by `this`.  The methods do not capture environment and do not act as closures.

9. Documenting
--------------

Documentation can be attached to functions, custom types, custom types their method and properties.

.. code::  

    /! documentation for function a
    let a = fn() -> null 
    {
        return null;
    }

    /! documentation for function b
    /! running over multiple lines
    let b = fn() -> null 
    {
        return null;
    }

    /! documentation for custom type Custom
    type Custom 
    {
        /! documentation for function Custom.c
        c = fn() -> null { return null; }
    };

The special syntax of `/!` is used to indicate the start of a documentation string.  Requesting the documentation programmatically 
is done through the `doc` function called on the object, type or method.

10. Modules
-----------

Modules can be used to organize code.  A module is loaded by the `import` statement.  When the module is the first time loaded all 
code in it is executed, including any top level statements.  When the module has been loaded, it can be loaded again, but the code will
not be executed twice or more.

Methods and variables in a module can be referenced using the `::` syntax.

.. code:: 

    import test_module;
    print("test_module::name=", test_module::name);

For above to work there needs to be a file named `test_module.luci` in the same directory where the code will execute.  Nesting of 
modules is allowed and will use the file system to organize.  
