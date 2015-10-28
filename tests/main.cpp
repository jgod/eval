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
                   const double val)
{
  std::map<std::string, double> vars;
  vars[var] = val;
  return eval(str, vars);
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

  SECTION("unary -")
  {
    SECTION("basic") {REQUIRE(eval("-1 + 3") == 2);}
    SECTION("further in parens") {REQUIRE(eval("((-5+3) * 8)") == -16);}
    SECTION("outside parens") {REQUIRE(eval("-(3*2)") == -6);}
    SECTION("in several contexts") {REQUIRE(eval("((-5+3) * (-8 + (-3 + 1)))") == 20);}
  }

  SECTION("unary +")
  {
    SECTION("basic") {REQUIRE(eval("+1 + 3") == 4);}
    SECTION("further in parens") {REQUIRE(eval("((+5+3) * 8)") == 64);}
    SECTION("outside parens") {REQUIRE(eval("+(3*2)") == 6);}
    SECTION("in several contexts") {REQUIRE(eval("((+5+3) * (+8 + (+3 + 1)))") == 96);}
  }

  SECTION("collapsing")
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

TEST_CASE("other symbols") {SECTION("parenthesis work") {REQUIRE(eval("((3*(2-(3))*4()))()") == -12);}}
