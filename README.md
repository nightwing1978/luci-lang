# Luci Language

Luci is an optionally typed scripting language.  It is developed in C++, has no external dependencies and is about learning what it takes to create a realistic (scripted) programming language.  It has its roots in https://interpreterbook.com/.

Luci has following:
* A syntax that is a mix of C, C++ and Python
* Supports imperative, object-oriented and functional style of programming
* Supports optional typing
* REPL

# Examples

## Hello world

```
let const words : [str] = [
    "Hello",
    "world"];

print(" ".join(words));
```

## Fibonacci

```
let fibonacci = fn(x : int) -> int {
    if (x<=1) {
        return x;
    }
    return fibonacci(x-1)+fibonacci(x-2)
}

fibonacci(10);
```

## Optional Typing

```
let words = [
    "Hello",
    "world"];

words[0] = 5.0;     // this is ok

let const const_words : [str] = [
    "Hello",
    "world"];

const_words[0] = 5.0;     // this is an error, because of const and type mismatch of double vs str
```

# Build and Run

Luci is built using cmake. A modern compiler supporting C++20 is required. 

1. Build

```
cmake --build build --config RelWithDebInfo 
make
```

2. Run

```
./build/interp/luci
```

# License

Luci is free software distributed under the Apache 2 License.
