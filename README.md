# eval

Eval is a simple, header-only, expression parser for C++.

## features

* basic operators `* / + - ^ %`
	- unary `+ -`
* variables
* decimals
* well-documented
* tests

## requirements

C++11 compiler

## usage

### basic

```cpp
#include "eval.h"

assert(eval("3*2 + 4") == 10);
```

### custom vars

```cpp
#include "eval.h"

std::map<std::string, double> vars;
vars["myvar"] = 2;
assert(eval("3*myvar + 4", vars) == 10);
```

### error handling

```cpp
#include "eval.h"

try
{
  return eval("3.14q59");
}
catch (const std::invalid_argument &e)
{
  ...
}
```

## impl details

* components
  - tokenizer
  - [Shunting-yard algorithm](https://en.wikipedia.org/wiki/Shunting-yard_algorithm) to build [RPN](https://en.wikipedia.org/wiki/Reverse_Polish_notation) queue
  - [RPN calculator](https://en.wikipedia.org/wiki/Reverse_Polish_notation#Postfix_algorithm)
* numbers == doubles
* null expressions return 0
* ignores whitespace
* single stack
* exceptions for error handling

## license

MIT