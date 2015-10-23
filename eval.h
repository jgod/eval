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

#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <queue>
#include <exception>
#include <functional>
#include <cmath>

#define EVAL_FPOP(q) q.front(); q.pop()
#define EVAL_TPOP(stack) stack.top(); stack.pop()
#define EVAL_MOVE_TOP(from, to) to.push(from.top()); from.pop()

#pragma mark errors
#define EVAL_UNREC_TOKEN(token) std::invalid_argument("Unrecognized token type for symbol: '" + token + "'!");
#define EVAL_UNDEFINED_VAR(token) std::invalid_argument("Undefined variable: '" + token + "'!");
#define EVAL_MISMATCHED_PARENS std::invalid_argument("There are mismatched parenthesis!");
#define EVAL_INVALID_EXPR std::domain_error("Invalid expression!");
#define EVAL_UNREC_OP std::domain_error("Unknown operator!");
#define EVAL_INPUT_TOO_MANY_VALS std::invalid_argument("Input has too many values!");
#pragma mark

namespace _eval
{
#pragma mark - Types
  typedef double ValType;
  typedef std::queue<std::string> Queue;
  typedef std::map<std::string, ValType> ValMap;
  typedef std::vector<std::string> Tokens;

  typedef struct
  {
    // Keep track of whether a number already exists in the context. This is so
    // we can properly treat the first number if it has a prepended +/-.
    bool inContext = false;
    // Keep track of whether we've encountered a decimal point, since there can
    // only be one per number.
    bool hasDecimal = false;
  } NumberFlags;

#pragma mark - Type Checking
  namespace Type
  {
    bool isNumber(const std::string &str)
    {
      if (str.empty()) {return false;}
      // The most common case is whole numbers.
      auto it = std::find_if(std::begin(str),
                             std::end(str),
                             [](const char c) {return !std::isdigit(c);});
      // If it's not a number by the digit definition, then try casting it to a double.
      // If it's castable, then it's a number.
      if (it != std::end(str))
      {
        try {std::stod(str); return true;}
        catch (const std::exception &e) {return false;}
      }
      return true;
    }

    bool isNumber(const char c) {return isNumber(std::string(1, c));}
    bool isLetter(const char c) {return std::isalpha(c);}
    bool containsLettersOnly(const std::string &str)
    {
      return std::find_if(std::begin(str),
                          std::end(str),
                          [](const char c) {return !std::isalpha(c);}) == std::end(str);
    }

    bool isOperator(const std::string &str)
    {
      return !str.empty() && (
        (str == "^") || (str == "*") || (str == "/")
        || (str == "+") || (str == "-") || (str == "%"));
    }
    bool isOperator(const char c) {return isOperator(std::string(1, c));}
    bool isParenthesis(const std::string &str) {return (str == "(" || str == ")");}
    bool isParenthesis(const char c) {return isParenthesis(std::string(1, c));}
  }

#pragma mark - Operator Handling
  namespace Op
  {
    int getPriority(const std::string &str)
    {
      if (str == "*" || str == "/" || str == "%") {return 3;}
      else if (str == "+" || str == "-") {return 2;}
      else if (str == "^") {return 4;}
      return -1;
    }

    bool isRHS(const std::string &str) {return (str == "^");}
    bool isLHS(const std::string &str) {return !isRHS(str);}
    bool validUnary(const char c) {return (c == '-') || (c == '+');}
  }

#pragma mark - Evaluation
  /**
   Gets tokens from an expression.

   @param exp Source string to evaluate
   @returns tokens
   */
  const Tokens tokenize(std::string exp)
  {
    Tokens tokens;
    // Re-use this string to build multi-character tokens between iterations.
    std::string str = "";
    // Numbers have a few edge cases that need to be tracked while we're parsing.
    NumberFlags number;

    // Since we only handle one character at a time, as soon as we know enough
    // about which token the current character is in, we can say for sure that the last
    // token (if any) is done being built by calling FINISH_PREV.
    const auto FINISH_PREV = [&]()
    {
      if (!str.empty()) {tokens.push_back(str);}
      str = "";
      number.hasDecimal = false;
    };
    const auto SINGLE_CHAR_IMPL = [&](const char c) {FINISH_PREV(); tokens.emplace_back(std::string(1, c));};
    // All multi-character tokens are built the same way, the only thing that
    // differs is how the token should be validated.
    const auto MULTI_CHAR_IMPL = [&](const char c, std::function<bool()> validates)
    {
      if (str.empty() || (!str.empty() && validates())) {str += c;}
      else {FINISH_PREV(); tokens.emplace_back(std::string(1, c));}
    };

    for (const auto c : exp)
    {
      if (c == ' ') {continue;}

      // Single-character only tokens: operators and parenthesis
      if (Type::isOperator(c) || Type::isParenthesis(c))
      {
        // At the start of a new context, reset any context flags.
        if (c == '(') {number.inContext = false;}
        // If a valid unary (+/-) is before any numbers in a context (e.g. (-2 + 3)),
        // prepend an explicit 0 as an easy solution.
        else if (Op::validUnary(c) && str.empty() && !number.inContext) {tokens.push_back("0");}
        SINGLE_CHAR_IMPL(c);
      }
      // Possible multi-character tokens: numbers and vars
      else
      {
        if (Type::isNumber(c))
        {
          MULTI_CHAR_IMPL(c, [&](){return Type::isNumber(str);});
          number.inContext = true;
        }
        else if (c == '.')
        {
          MULTI_CHAR_IMPL(c, [&]()
          {
            // A dot is only valid when the string currently being built is a number,
            // and that number doesn't already have a decimal.
            const auto valid = Type::isNumber(str) && !number.hasDecimal;
            if (valid) {number.hasDecimal = true;}
            return valid;
          });
        }
        else if (Type::isLetter(c))
        {
          MULTI_CHAR_IMPL(c, [&](){return Type::containsLettersOnly(str);});
        }
        else
        {
          FINISH_PREV();
        }
      }
    }

    FINISH_PREV();
    return tokens;
  }

  /**
   Reads a string into RPN, following the Shunting-yard algorithm.

   @see [Shunting-yard](https://en.wikipedia.org/wiki/Shunting-yard_algorithm )
   @param str Source string to evaluate
   @param vars Map of variables to look up otherwise undefined tokens (optional)
   @returns Output queue
   */
  Queue read(const std::string &str, ValMap vars = ValMap())
  {
    Queue q;
    std::stack<std::string> opStack;
    auto tokens = tokenize(str);

    // While there are tokens to be read, read a token.
    for (const auto token : tokens)
    {
      // If the token is a number, then add it to the output queue.
      if (Type::isNumber(token)) {q.push(token);}

      // If the token evaluates to a number, then add it to the output queue.
      else if (vars[token]) {q.push(std::to_string(vars[token]));}

      // (unimplemented) If the token is a function token, then push it onto the stack.
      // (unimplemented) If the token is a function argument separator (e.g., a comma):
      // Until the token at the top of the stack is a left parenthesis, pop operators
      // off the stack onto the output queue. If no left parentheses are encountered,
      // either the separator was misplaced or parentheses were mismatched.

      // If the token is an operator, o1, then:
      else if (Type::isOperator(token))
      {
        // while there is an operator token, o2, at the top of the operator stack,
        while (!opStack.empty()
               // and either o1 is left-associative and its precedence is less than or equal to that of o2,
               && (
                   (Op::isLHS(token) && (Op::getPriority(token) <= Op::getPriority(opStack.top())))
                   ||
                   // or o1 is right associative, and has precedence less than that of o2,
                   (Op::isRHS(token) && (Op::getPriority(token) < Op::getPriority(opStack.top())))
                   )
               )
        {
          // then pop o2 off the operator stack, onto the output queue;
          EVAL_MOVE_TOP(opStack, q);
        }
        // push o1 onto the operator stack.
        opStack.push(token);
      }
      // If the token is a left parenthesis (i.e. "("), then push it onto the stack.
      else if (token == "(") {opStack.push(token);}
      // If the token is a right parenthesis (i.e. ")"):
      else if (token == ")")
      {
        // Until the token at the top of the stack is a left parenthesis,
        // pop operators off the stack onto the output queue.
        while (!opStack.empty() && opStack.top() != "(") {EVAL_MOVE_TOP(opStack, q);}

        // Pop the left parenthesis from the stack, but not onto the output queue.
        if (!opStack.empty() && opStack.top() == "(") {opStack.pop();}
        // (unimplemented) If the token at the top of the stack is a function token, pop it onto the output queue.
        // If the stack runs out without finding a left parenthesis, then there are mismatched parentheses.
        else
        {
          throw EVAL_MISMATCHED_PARENS;
        }
      }
      // Handle exceptions
      else
      {
        // Strings should have been evaluated as variables (macro-like).
        // If the token is still only letters then that means that the variable
        // is undefined.
        if (Type::containsLettersOnly(token)) {throw EVAL_UNDEFINED_VAR(token);}
        else {throw EVAL_UNREC_TOKEN(token);}
      }
    }

    // When there are no more tokens to read:
    // While there are still operator tokens in the stack:
    while (!opStack.empty())
    {
      // If the operator token on the top of the stack is a parenthesis,
      // then there are mismatched parentheses.
      if (Type::isParenthesis(opStack.top())) {throw EVAL_MISMATCHED_PARENS;}
      // Pop the operator onto the output queue.
      EVAL_MOVE_TOP(opStack, q);
    }
    return q;
  }

  /**
   Evaluates an RPN expression.

   @see [Postfix algorithm](https://en.wikipedia.org/wiki/Reverse_Polish_notation#Postfix_algorithm )
   @param queue
   @returns ValType
   */
  ValType queue(Queue &q)
  {
    std::stack<ValType> valStack;

    // While there are input tokens left, read the next token from input.
    while (!q.empty())
    {
      auto &token = EVAL_FPOP(q);

      // If the token is a value, push it on the stack.
      if (Type::isNumber(token))
      {
        valStack.push(std::stod(token));
      }
      // Otherwise, the token is an operator (operator here includes both operators and functions).
      else
      {
        // It is known a priori that the operator takes n arguments.
        // If there are fewer than n values on the stack
        // (Error) The user has not input sufficient values in the expression.
        if (valStack.size() < 2) {throw EVAL_INVALID_EXPR;}
        // Else, Pop the top n values from the stack.
        const auto R = EVAL_TPOP(valStack);
        const auto L = EVAL_TPOP(valStack);

        // Evaluate the operator, with the values as arguments.
        // Push the returned results, if any, back onto the stack.
        if (token == "*") {valStack.push(L*R);}
        else if (token == "/") {valStack.push(L/R);}
        else if (token == "+") {valStack.push(L + R);}
        else if (token == "-") {valStack.push(L - R);}
        else if (token == "^") {valStack.push(pow(L, R));}
        else if (token == "%") {valStack.push(static_cast<int>(L) % static_cast<int>(R));}
        else {throw EVAL_UNREC_OP;}
      }
    }

    // If there is only one value in the stack, that value is the result of the calculation.
    if (valStack.size() == 1) {return valStack.top();}
    // Otherwise, there are more values in the stack
    // (Error) The user input has too many values.
    else {throw EVAL_INPUT_TOO_MANY_VALS;}
  }
}

/**
 Evaluates a string by tokenizing, creating an RPN stack, and evaluating it.

 @param str
 @param vars Custom in-place variables (optional)
 @returns result
 */
_eval::ValType eval(std::string str, _eval::ValMap vars = _eval::ValMap())
{
  // Remove whitespace.
  str.erase(std::remove(std::begin(str), std::end(str), ' '), std::end(str));
  if (str.empty()) return 0;

  auto q = _eval::read(str, vars);
  return _eval::queue(q);
}
#endif /* eval_h */
