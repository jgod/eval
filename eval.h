/*
The MIT License (MIT)

Copyright (c) 2015 Justin Godesky

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef eval_h
#define eval_h

#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <queue>
#include <exception>
#include <functional>

//// Dragons:
#pragma mark - Probably Shouldn't be Macros
// Add our own JavaScript-like pop() functions for clarity.
#define EVAL_FPOP(q) q.front(); q.pop()
#define EVAL_TPOP(stack) stack.top(); stack.pop()
#define EVAL_MOVE_TOP(from, to) to.push(from.top()); from.pop()
//
#define EVAL_MOVE_TOP_UNTIL_LEFT_PAREN(from, to) while (!from.empty() && from.top() != "(") {EVAL_MOVE_TOP(from, to);}

#pragma mark - Errors
#define EVAL_UNREC_TOKEN(token) std::invalid_argument("Unrecognized token type for symbol: \"" + token + "\"!")
#define EVAL_UNDEFINED_VAR(token) std::invalid_argument("Undefined variable: \"" + token + "\"!")
#define EVAL_MISMATCHED_PARENS std::invalid_argument("There are mismatched parenthesis!")
#define EVAL_INVALID_EXPR std::domain_error("Invalid expression!")
#define EVAL_UNREC_OP std::domain_error("Unknown operator!")
#define EVAL_INPUT_TOO_MANY_VALS std::invalid_argument("Input has too many values!")
// Functions
#define EVAL_INVALID_FN_INVOCATION std::invalid_argument("Invalid function invocation!")
#define EVAL_INVALID_FN_NUMBER_ARGS(fn, expected, received) \
std::domain_error("Function "#fn" has wrong number of arguments! Expected " + std::to_string(expected) + " but got " + std::to_string(received) + ".")
#define EVAL_INVALID_ARG_TYPE(arg) std::invalid_argument("Function arg " + arg + " is wrong type!")
#pragma mark

#pragma mark - Function Bindings
#define EVAL_CHECK_ARG_NUMBER(idx) if (!Type::isNumber(args[idx])) {throw EVAL_INVALID_ARG_TYPE(args[idx]);}
#define EVAL_ARG_NUMBER(idx) Type::toNumber(args[idx]) // Cast value at index to number
#define EVAL_BIND_INCL_FN(name) fns[#name] = std::bind(&_eval::Builtins:: name , std::placeholders::_1); // Bind an included function
#define EVAL_DEF_INCL_FN(name) Number name(FnArgs args) {EVAL_FN_IMPL_NUMBER(#name, std:: name);} // Define an included function
#define EVAL_FN_IMPL_NUMBER(name, fn) \
if (args.empty()) {throw EVAL_INVALID_FN_NUMBER_ARGS(name, 1, 0);} \
else if (args.size() > 1) {throw EVAL_INVALID_FN_NUMBER_ARGS(name, 1, args.size());} \
EVAL_CHECK_ARG_NUMBER(0) \
return fn(EVAL_ARG_NUMBER(0));
//
#define EVAL_FN_IMPL_2NUMBERS(name, fn) \
if (args.empty()) {throw EVAL_INVALID_FN_NUMBER_ARGS(name, 2, 0);} \
else if (args.size() != 2) {throw EVAL_INVALID_FN_NUMBER_ARGS(name, 2, args.size());} \
EVAL_CHECK_ARG_NUMBER(0) \
EVAL_CHECK_ARG_NUMBER(1) \
return fn(EVAL_ARG_NUMBER(0), EVAL_ARG_NUMBER(1));
#pragma mark
////

namespace _eval { // Make it clear that this is an implementation.
#pragma mark - Types
  // Strings are chosen as the base value type since it can both a string and a number (through casting).
  typedef std::string BaseVal;

  // These typedefs are redundant in practice only because everything shares the
  // same type, but it allows writing rules in a little more DSL-like way.
  //
  // Expression-allowed types (Number, String)
  typedef double Number; // Doubles provide good precision; use this as the base evaluation type, for now.
  // std::string String
  // Meta
  typedef char SubToken;
  typedef std::string Token;
  typedef Token OpType;
  typedef std::queue<Token> Queue;
  typedef std::map<std::string, Number> VarMap;
  typedef std::vector<Token> Tokens;
  //

  struct NumberFlags {
    // Whether a number already exists in the context. This is so we can handle first numbers with prepended +/-.
    bool inContext = false;
    // Whether we've encountered a decimal point, since there can only be one per number.
    bool hasDecimal = false;
  };

  // "Function" impl (implementer has to do a bit of work, doesn't autobind to C++)
  typedef BaseVal ArgType; // Args must derive from the base type
  typedef std::vector<ArgType> FnArgs; // Variable args handled by passing vector
  typedef std::function<Number(FnArgs)> Fn; // Functions only have one return type, for now.
  typedef std::map<std::string, Fn> FnMap;

#pragma mark - Type Checking
  namespace Type {
    Token toToken(const Number n) {return std::to_string(n);}
    Number toNumber(const BaseVal &s) {return std::stod(s);}
    bool isNumber(const Token &s) {
      if (s.empty()) return false;
      // The most common case is whole numbers (where every char is a digit).
      auto it = std::find_if(std::begin(s), std::end(s), [](const SubToken c) {return !std::isdigit(c);});
      // If it's not a number by that definition, then try casting it to a double.
      // If it's castable, then it's a number. This is to support decimals.
      if (it != std::end(s)) try {toNumber(s); return true;} catch (const std::exception&) {return false;}
      return true;
    }

    bool isNumber(const SubToken c) {return isNumber(Token(1, c));}
    bool isLetter(const SubToken c) {return std::isalpha(c);}
    bool containsLettersOnly(const Token &s) {
      return std::find_if(std::begin(s), std::end(s), [](const SubToken c) {return !std::isalpha(c);}) == std::end(s);
    }
    bool isOperator(const Token &s) {return !s.empty() && ((s=="+")||(s=="-")||(s=="*")||(s=="/")||(s=="^")||(s=="%"));}
    bool isOperator(const SubToken c) {return isOperator(Token(1, c));}
    bool isParenthesis(const Token &s) {return s == "(" || s == ")";}
    bool isParenthesis(const SubToken c) {return isParenthesis(Token(1, c));}
    bool isFunctionSeperator(const Token &s) {return s == ",";}
    bool isFunctionSeperator(const SubToken c) {return isFunctionSeperator(Token(1, c));}
  }

#pragma mark - Operator Checking
  namespace Op {
    int getPriority(const OpType &s) {
      if (s == "*" || s == "/" || s == "%") return 3;
      else if (s == "+" || s == "-") return 2;
      else if (s == "^") return 4;
      return -1;
    }

    bool isRHS(const OpType &s) {return s == "^";}
    bool isLHS(const OpType &s) {return !isRHS(s);}
    bool isUnary(const OpType &s) {return s == "-" || s == "+";}
    bool isUnary(const SubToken c) {return isUnary(Token(1, c));}
  }

#pragma mark - Builtins
  namespace Builtins {
    // Vars
    Number pi() {return atan(1.0) * 4.0;}
    // Functions
    EVAL_DEF_INCL_FN(abs);
    EVAL_DEF_INCL_FN(sqrt); EVAL_DEF_INCL_FN(cbrt);
    EVAL_DEF_INCL_FN(sin); EVAL_DEF_INCL_FN(cos); EVAL_DEF_INCL_FN(tan);
    EVAL_DEF_INCL_FN(asin); EVAL_DEF_INCL_FN(acos); EVAL_DEF_INCL_FN(atan);
    EVAL_DEF_INCL_FN(floor); EVAL_DEF_INCL_FN(ceil); EVAL_DEF_INCL_FN(trunc); EVAL_DEF_INCL_FN(round);
    Number hypot(FnArgs args) {EVAL_FN_IMPL_2NUMBERS("hypot", std::hypot);}
  }

#pragma mark - Utils
  std::string replaceAll(std::string s, const std::string &search, const std::string &r) {
    size_t pos = 0;
    while ((pos = s.find(search, pos)) != std::string::npos) {
      s.replace(pos, search.length(), r);
      pos += r.length();
    }
    return s;
  }

#pragma mark - Evaluation
  /**
   Rewrites an expression to something that can be tokenized easier.

   @param[in] exp
   @returns expression
   */
  std::string rewriteExpression(std::string exp) {
    // Rewriting adjacent operators:
    // Unary operators next to binary operators of the same symbol can easily be
    // rewritten throughout the whole string all at once.
    exp = replaceAll(exp, "+-", "-");
    exp = replaceAll(exp, "-+", "-");
    exp = replaceAll(exp, "++", "+");
    exp = replaceAll(exp, "--", "+");
    return exp;
  }

  /**
   Gets tokens from an expression.

   @see what's supported by reading the tests
   @param[in] exp Source string to evaluate
   @returns tokens
   */
  const Tokens tokenize(std::string exp) {
    Tokens toks;
    Token wip = ""; // Re-use this string to build multi-character tokens between iterations.
    NumberFlags number; // Numbers have edge cases that need to be tracked while we're parsing.

    // Since we only handle one character at a time, as soon as we know enough
    // about which token the current character is in, we can say for sure that the last
    // token (if any) is done being built by calling FINISH_PREV.
    const auto FINISH_PREV = [&]() {if (!wip.empty()) {toks.push_back(wip);} wip = ""; number.hasDecimal = false;};
    const auto SINGLE_CHAR_IMPL = [&](const SubToken c) {FINISH_PREV(); toks.emplace_back(Token(1, c));};
    // All multi-character tokens are built the same way, the only thing that
    // differs is how the token should be validated.
    const auto MULTI_CHAR_IMPL = [&](const SubToken c, std::function<bool()> validates) {
      if (wip.empty() || validates()) wip += c;
      else {FINISH_PREV(); toks.emplace_back(Token(1, c));}
    };

    for (const auto c : exp) { if (c == ' ') continue;
      // Single-character only tokens: operators and parenthesis
      if (Type::isOperator(c) || Type::isParenthesis(c) || Type::isFunctionSeperator(c)) {
        if (c == '(') {number.inContext = false;} // Starting new context, reset any flags.
        else if (Op::isUnary(c) && wip.empty() && !number.inContext) {
          // If a valid unary (+/-) is before any numbers in a context (e.g. (-2 + 3)),
          // prepend an explicit 0 as an easy solution.
          toks.push_back("0"); number.inContext = true;
        }
        SINGLE_CHAR_IMPL(c);
      } else {
        // Possible multi-character tokens: numbers and vars/functions
        if (Type::isNumber(c)) {
          MULTI_CHAR_IMPL(c, [&](){return Type::isNumber(wip);});
          number.inContext = true;
        } else if (c == '.') {
          MULTI_CHAR_IMPL(c, [&]() {
            // A dot is only valid when the string currently being built is a number,
            // and that number doesn't already have a decimal.
            const auto valid = Type::isNumber(wip) && !number.hasDecimal;
            if (valid) number.hasDecimal = true;
            return valid;
          });
        }
        else if (Type::isLetter(c)) {MULTI_CHAR_IMPL(c, [&](){return Type::containsLettersOnly(wip);});}
        else {FINISH_PREV();}
      }
    }

    FINISH_PREV();
    return toks;
  }

  /**
   Reads a string into RPN, following the Shunting-yard algorithm.

   @see [Shunting-yard](https://en.wikipedia.org/wiki/Shunting-yard_algorithm )
   @param[in] str Source string to evaluate
   @param[in] vars Map of variables to look up (optional)
   @param[in] fns Map of functions to look up (optional)
   @returns Output queue
   */
  Queue read(const std::string &str, VarMap vars = VarMap(), FnMap fns = FnMap()) {
    Queue q;
    std::stack<Token> opStack;
    auto tokens = tokenize(str);

    // Function handling
    std::string fnToInvoke = "";
    bool expectingArg = false;
    FnArgs fnArgs;

    for (const auto token : tokens) { // While there are tokens to be read, read a token.
      if (expectingArg && token != ")") {
        fnArgs.push_back(vars.count(token) ? Type::toToken(vars[token]) : token);
        expectingArg = false;
        continue;
      }
      if (Type::isNumber(token)) q.push(token); // If the token is a number, then add it to the output queue.
      // If the token evaluates to a number, then add it to the output queue.
      else if (vars.count(token)) q.push(Type::toToken(vars[token]));
      // If the token is a function token, mark it as currently being evaluated
      else if (fns.count(token)) fnToInvoke = token;
      // If the token is a function argument separator (e.g., a comma):
      else if (Type::isFunctionSeperator(token)) expectingArg = true;
      else if (Type::isOperator(token)) { // If the token is an operator, o1, then:
        while (!opStack.empty() // while there is an operator token, o2, at the top of the operator stack,
               // and either o1 is left-associative and its precedence is less than or equal to that of o2,
               && (
                   (Op::isLHS(token) && (Op::getPriority(token) <= Op::getPriority(opStack.top())))
                   ||
                   // or o1 is right associative, and has precedence less than that of o2,
                   (Op::isRHS(token) && (Op::getPriority(token) < Op::getPriority(opStack.top())))
                   )
               ) {
          EVAL_MOVE_TOP(opStack, q); // then pop o2 off the operator stack, onto the output queue;
        }
        opStack.push(token); // push o1 onto the operator stack.
      }
      else if (token == "(") {
        if (!fnToInvoke.empty()) {expectingArg = true; continue;}
        opStack.push(token); // If the token is a left parenthesis (i.e. "("), then push it onto the stack.
      }
      else if (token == ")") {
        if (!fnToInvoke.empty()) {
          q.push(Type::toToken(fns[fnToInvoke](fnArgs)));
          fnArgs.clear();
          fnToInvoke = "";
          expectingArg = false;
          continue;
        }
        // If the token is a right parenthesis (i.e. ")"):
        // Until the token at the top of the stack is a left parenthesis,
        // pop operators off the stack onto the output queue.
        EVAL_MOVE_TOP_UNTIL_LEFT_PAREN(opStack, q);

        // Pop the left parenthesis from the stack, but not onto the output queue.
        if (!opStack.empty() && opStack.top() == "(") opStack.pop();
        // If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
        else throw EVAL_MISMATCHED_PARENS;
      } else if (Type::containsLettersOnly(token)) { // Strings should have been evaluated as variables (macro-like).
        // If the token is still only letters then that means that the variable
        // is undefined.
        throw EVAL_UNDEFINED_VAR(token);
      } else throw EVAL_UNREC_TOKEN(token);
    } // When there are no more tokens to read:

    while (!opStack.empty()) { // While there are still operator tokens in the stack:
      // If the operator token on the top of the stack is a parenthesis,
      // then there are mismatched parentheses.
      if (Type::isParenthesis(opStack.top())) throw EVAL_MISMATCHED_PARENS;
      EVAL_MOVE_TOP(opStack, q); // Pop the operator onto the output queue.
    }
    return q;
  }

  /**
   Evaluates an RPN expression.

   @see [Postfix algorithm](https://en.wikipedia.org/wiki/Reverse_Polish_notation#Postfix_algorithm )
   @param[out] queue
   @returns Number
   */
  Number queue(Queue &q) {
    std::stack<Number> valStack;

    while (!q.empty()) { // While there are input tokens left, read the next token from input.
      auto &token = EVAL_FPOP(q);

      if (Type::isNumber(token)) valStack.push(Type::toNumber(token)); // If the token is a value, push it on the stack.
      else { // Otherwise, the token is an operator.
        // It is known a priori that the operator takes n arguments.
        // If there are fewer than n values on the stack
        if (valStack.size() < 2) throw EVAL_INVALID_EXPR; // (Error) The user has not input sufficient values in the expression.
        // Else, Pop the top n values from the stack.
        const auto R = EVAL_TPOP(valStack);
        const auto L = EVAL_TPOP(valStack);

        // Evaluate the operator, with the values as arguments.
        // Push the returned results, if any, back onto the stack.
        if (token == "*") valStack.push(L*R);
        else if (token == "/") valStack.push(L/R);
        else if (token == "+") valStack.push(L + R);
        else if (token == "-") valStack.push(L - R);
        else if (token == "^") valStack.push(std::pow(L, R));
        else if (token == "%") valStack.push(static_cast<int>(L) % static_cast<int>(R));
        else throw EVAL_UNREC_OP;
      }
    }
    if (valStack.size() == 1) return valStack.top(); // If there is only one value in the stack, that value is the result of the calculation.
    else throw EVAL_INPUT_TOO_MANY_VALS; // Otherwise, there are more values in the stack: (Error) The user input has too many values.
  }

  /**
   Binds built-in functions to variable and function maps.

   @param[out] vars
   @param[out] fns
   */
  void bindBuiltins(_eval::VarMap &vars, _eval::FnMap &fns) {
    vars["pi"] = _eval::Builtins::pi();
    EVAL_BIND_INCL_FN(abs);
    EVAL_BIND_INCL_FN(sqrt); EVAL_BIND_INCL_FN(cbrt);
    EVAL_BIND_INCL_FN(sin); EVAL_BIND_INCL_FN(cos); EVAL_BIND_INCL_FN(tan);
    EVAL_BIND_INCL_FN(asin); EVAL_BIND_INCL_FN(acos); EVAL_BIND_INCL_FN(atan);
    EVAL_BIND_INCL_FN(floor); EVAL_BIND_INCL_FN(ceil); EVAL_BIND_INCL_FN(trunc); EVAL_BIND_INCL_FN(round);
    EVAL_BIND_INCL_FN(hypot);
  }
}

/**
 Evaluates a string by tokenizing, creating an RPN stack, and evaluating it.
 Note that this is a free function.

 @param[in] str
 @param[in] vars Custom in-place variables (optional)
 @param[in] fns Functions (optional)
 @returns result
 */
_eval::Number eval(std::string str,
                   _eval::VarMap vars = _eval::VarMap(),
                   _eval::FnMap fns = _eval::FnMap()) {
  str.erase(std::remove(std::begin(str), std::end(str), ' '), std::end(str)); // Remove whitespace.
  if (str.empty()) return 0; // "null" evaluates to 0.
  _eval::bindBuiltins(vars, fns);
  auto q = _eval::read(_eval::rewriteExpression(str), vars, fns);
  return _eval::queue(q);
}
#endif /* eval_h */
