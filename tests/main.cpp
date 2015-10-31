//
//  main.cpp
//  eval
//
//  Created by Justin Godesky on 10/20/15.
//
//

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../eval.h"

double evalWithVar(const std::string &str,
                   const std::string &var,
                   const _eval::Number val)
{
  _eval::VarMap vars;
  vars[var] = val;
  return eval(str, vars);
}

double evalWithFn(const std::string &str,
                  const std::string &fnName,
                  const _eval::Number val)
{
  _eval::FnMap fns;
  fns[fnName] = [=](_eval::FnArgs args) {return val;};
  return eval(str, _eval::VarMap(), fns);
}

TEST_CASE("basic evaluation")
{
  SECTION("nothing") {REQUIRE(eval("") == 0);}
  SECTION("only whitespace") {REQUIRE(eval("     ") == 0);}
  SECTION("number") {REQUIRE(eval("2") == 2);}

  SECTION("decimals")
  {
    SECTION("basic") {REQUIRE(eval("2.5") == 2.5f);}
    SECTION("operations") {REQUIRE(eval("2.5*2 + 1.75") == 6.75f);}
  }

  SECTION("ignores whitespace")
  {
    REQUIRE(eval("3 + 4*2 + 6") == 17);
    REQUIRE(eval("3+4   * 2 +   6") == 17);
    REQUIRE(eval("3+4*2+6") == 17);
  }
}

TEST_CASE("variables")
{
  SECTION("custom") {REQUIRE(evalWithVar("myvar", "myvar", 2) == 2);}
  SECTION("evals like any other number") {REQUIRE(evalWithVar("3 + myvar*3 - 2", "myvar", 5) == 16);}
}

TEST_CASE("operators")
{
  SECTION("+ operator") {REQUIRE(eval("2 + 3") == 5);}
  SECTION("- operator") {REQUIRE(eval("2 + 2") == 4);}
  SECTION("* operator") {REQUIRE(eval("2*2") == 4);}
  SECTION("/ operator") {REQUIRE(eval("2/4") == 0.5);}
  SECTION("^(power) operator") {REQUIRE(eval("2^3") == 8);}
  SECTION("% operator") {REQUIRE(eval("5%2") == 1);}

  SECTION("unaries")
  {
    SECTION("-")
    {
      SECTION("basic") {REQUIRE(eval("-1 + 3") == 2);}
      SECTION("further in parens") {REQUIRE(eval("((-5+3) * 8)") == -16);}
      SECTION("outside parens") {REQUIRE(eval("-(3*2)") == -6);}
      SECTION("in several contexts") {REQUIRE(eval("((-5+3) * (-8 + (-3 + 1)))") == 20);}
    }

    SECTION("+")
    {
      SECTION("basic") {REQUIRE(eval("+1 + 3") == 4);}
      SECTION("further in parens") {REQUIRE(eval("((+5+3) * 8)") == 64);}
      SECTION("outside parens") {REQUIRE(eval("+(3*2)") == 6);}
      SECTION("in several contexts") {REQUIRE(eval("((+5+3) * (+8 + (+3 + 1)))") == 96);}
    }

    SECTION("rewrite adjacent")
    {
      SECTION("in middle of expression")
      {
        REQUIRE(eval("1 + - 3") == -2);
        REQUIRE(eval("1 - + 3") == -2);
        REQUIRE(eval("1 - - 3") == 4);
        REQUIRE(eval("1 + + 3") == 4);
      }

      SECTION("in front of expression")
      {
        REQUIRE(eval("+- 3") == -3);
        REQUIRE(eval("-+ 3") == -3);
        REQUIRE(eval("-- 3") == 3);
        REQUIRE(eval("++ 3") == 3);
        REQUIRE(eval("++(3-2)") == 1);
        REQUIRE(eval("+-(3-2)") == -1);
      }
    }
  }
}

TEST_CASE("other symbols")
{
  SECTION("parenthesis work") {REQUIRE(eval("((3*(2-(3))*4()))()") == -12);}
}

TEST_CASE("functions")
{
  SECTION("no args") {REQUIRE(evalWithFn("function()", "function", 1) == 1);}
  SECTION("builtins")
  {
    SECTION("var") {SECTION("pi") {REQUIRE(floor(eval("pi")*100 + 0.5f)/100.00 == 3.14);}}
    SECTION("functions")
    {
      SECTION("math")
      {
        SECTION("abs") {REQUIRE(eval("abs(-3)") == 3);}
        SECTION("sqrt") {REQUIRE(floor(eval("sqrt(2)")*100 + 0.5f)/100.00 == 1.41);}
        SECTION("cbrt") {REQUIRE(eval("cbrt(27)") == 3);}
        SECTION("sin") {REQUIRE(eval("sin(pi)") == 0);}
        SECTION("cos") {REQUIRE(eval("cos(pi)") == -1);}
        SECTION("tan") {REQUIRE(eval("tan(pi)") == 0);}
        SECTION("asin") {REQUIRE(eval("asin(0)") == 0);}
        SECTION("acos") {REQUIRE(eval("acos(1)") == 0);}
        SECTION("atan") {REQUIRE(eval("atan(0)") == 0);}
        SECTION("floor") {REQUIRE(eval("floor(1.2)") == 1);}
        SECTION("ceil") {REQUIRE(eval("ceil(1.8)") == 2);}
        SECTION("trunc") {REQUIRE(eval("trunc(2.7)") == 2);}
        SECTION("round") {REQUIRE(eval("round(2.6)") == 3);}
        SECTION("hypot") {REQUIRE(eval("hypot(3, 4)") == 5);}
      }
    }
  }
}
