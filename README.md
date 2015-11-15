# eval

Eval is a simple, header-only, expression parser for C++.

## features

* easy to integrate - just include `eval.h`
* simple API: `eval("your expression", ?vars, ?functions)`
* operators `* / + - ^ %`
* numbers and strings
* variable argument functions
* built-in constants and functions like `pi`, `sqrt()`, `sin()`, `ceil()` etc.
* tests

## requirements

C++11 compiler

## usage

### basic

```cpp
#include "eval.h"
using namespace jgod;

assert(eval("3*2 + 4") == 10);
```

### vars

```cpp
std::map<std::string, double> vars;
vars["myvar"] = 2;
assert(eval("3*myvar + 4", vars) == 10);
```

### error handling

```cpp
try {return eval("3.14q59");}
catch (const std::invalid_argument &e) {...}
```

## impl details

* flow
  1. strip whitespace
  2. rewrite adjacent operators
  3. tokenize
  4. [Shunting-yard algorithm](https://en.wikipedia.org/wiki/Shunting-yard_algorithm) to build [RPN](https://en.wikipedia.org/wiki/Reverse_Polish_notation) queue
  5. process queue with [RPN calculator](https://en.wikipedia.org/wiki/Reverse_Polish_notation#Postfix_algorithm)
* values: numbers and strings
  - base value type is `std::string` (casts are necessary)
  - numbers == doubles
  - "null" expressions return 0
* unary `+ -`
* function binding using `std::function`
* variable length functions using `std::vector`
* `std::exception`s for error handling

## license

MIT