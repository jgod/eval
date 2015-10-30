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

// Add our own JavaScript-like pop() functions for clarity.
#define EVAL_FPOP(q) q.front(); q.pop()
#define EVAL_TPOP(stack) stack.top(); stack.pop()
#define EVAL_MOVE_TOP(from, to) to.push(from.top()); from.pop()

#pragma mark - Errors
#define EVAL_UNREC_TOKEN(token) std::invalid_argument("Unrecognized token type for symbol: '" + token + "'!");
#define EVAL_UNDEFINED_VAR(token) std::invalid_argument("Undefined variable: '" + token + "'!");
#define EVAL_MISMATCHED_PARENS std::invalid_argument("There are mismatched parenthesis!");
#define EVAL_INVALID_EXPR std::domain_error("Invalid expression!");
#define EVAL_UNREC_OP std::domain_error("Unknown operator!");
#define EVAL_INPUT_TOO_MANY_VALS std::invalid_argument("Input has too many values!");
#pragma mark

namespace _eval { // The implementation isn't really designed to be exposed.
#pragma mark - Types
  typedef double ValType;
  typedef std::queue<std::string> Queue;
  typedef std::map<std::string, ValType> ValMap;
  typedef std::vector<std::string> Tokens;

  typedef struct {
    // Whether a number already exists in the context. This is so we can handle first numbers with prepended +/-.
    bool inContext = false;
    // Whether we've encountered a decimal point, since there can only be one per number.
    bool hasDecimal = false;
  } NumberFlags;

#pragma mark - Type Checking
  namespace Type {
    bool isNumber(const std::string &s) {
      if (s.empty()) {return false;}
      // The most common case is whole numbers (where every char is a digit).
      auto it = std::find_if(std::begin(s), std::end(s), [](const char c) {return !std::isdigit(c);});
      // If it's not a number by that definition, then try casting it to a double.
      // If it's castable, then it's a number. This is to support decimals.
      if (it != std::end(s)) try {std::stod(s); return true;} catch (const std::exception &e) {return false;}
      return true;
    }

    bool isNumber(const char c) {return isNumber(std::string(1, c));}
    bool isLetter(const char c) {return std::isalpha(c);}
    bool containsLettersOnly(const std::string &s) {
      return std::find_if(std::begin(s), std::end(s), [](const char c) {return !std::isalpha(c);}) == std::end(s);
    }

    bool isOperator(const std::string &s) {
      return !s.empty() && ((s == "^")||(s == "*")||(s == "/")||(s == "+")||(s == "-")||(s == "%"));
    }
    bool isOperator(const char c) {return isOperator(std::string(1, c));}
    bool isParenthesis(const std::string &s) {return s == "(" || s == ")";}
    bool isParenthesis(const char c) {return isParenthesis(std::string(1, c));}
  }

#pragma mark - Operator Handling
  namespace Op {
    int getPriority(const std::string &s) {
      if (s == "*" || s == "/" || s == "%") return 3;
      else if (s == "+" || s == "-") return 2;
      else if (s == "^") return 4;
      return -1;
    }

    bool isRHS(const std::string &s) {return s == "^";}
    bool isLHS(const std::string &s) {return !isRHS(s);}
    bool isUnary(const char c) {return c == '-' || c == '+';}
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
    // Operator collapsing:
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
    std::string s = ""; // Re-use this string to build multi-character tokens between iterations.
    NumberFlags number; // Numbers have edge cases that need to be tracked while we're parsing.

    // Since we only handle one character at a time, as soon as we know enough
    // about which token the current character is in, we can say for sure that the last
    // token (if any) is done being built by calling FINISH_PREV.
    const auto FINISH_PREV = [&]() {if (!s.empty()) {toks.push_back(s);} s = ""; number.hasDecimal = false;};
    const auto SINGLE_CHAR_IMPL = [&](const char c) {FINISH_PREV(); toks.emplace_back(std::string(1, c));};
    // All multi-character tokens are built the same way, the only thing that
    // differs is how the token should be validated.
    const auto MULTI_CHAR_IMPL = [&](const char c, std::function<bool()> validates) {
      if (s.empty() || (!s.empty() && validates())) s += c;
      else {FINISH_PREV(); toks.emplace_back(std::string(1, c));}
    };

    for (const auto c : exp) { if (c == ' ') continue;
      // Single-character only tokens: operators and parenthesis
      if (Type::isOperator(c) || Type::isParenthesis(c)) {
        if (c == '(') {number.inContext = false;} // Starting new context, reset any flags.
        else if (Op::isUnary(c) && s.empty() && !number.inContext) {
          // If a valid unary (+/-) is before any numbers in a context (e.g. (-2 + 3)),
          // prepend an explicit 0 as an easy solution.
          toks.push_back("0"); number.inContext = true;
        }
        SINGLE_CHAR_IMPL(c);
      } else {
        // Possible multi-character tokens: numbers and vars
        if (Type::isNumber(c)) {
          MULTI_CHAR_IMPL(c, [&](){return Type::isNumber(s);});
          number.inContext = true;
        } else if (c == '.') {
          MULTI_CHAR_IMPL(c, [&]() {
            // A dot is only valid when the string currently being built is a number,
            // and that number doesn't already have a decimal.
            const auto valid = Type::isNumber(s) && !number.hasDecimal;
            if (valid) number.hasDecimal = true;
            return valid;
          });
        }
        else if (Type::isLetter(c)) {MULTI_CHAR_IMPL(c, [&](){return Type::containsLettersOnly(s);});}
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
   @param[in] vars Map of variables to look up otherwise undefined tokens (optional)
   @returns Output queue
   */
  Queue read(const std::string &str, ValMap vars = ValMap())
  {
    Queue q;
    std::stack<std::string> opStack;
    auto tokens = tokenize(str);

    for (const auto token : tokens) { // While there are tokens to be read, read a token.
      if (Type::isNumber(token)) q.push(token); // If the token is a number, then add it to the output queue.
      // If the token evaluates to a number, then add it to the output queue.
      else if (vars[token]) q.push(std::to_string(vars[token]));

      // (unimplemented) If the token is a function token, then push it onto the stack.
      // (unimplemented) If the token is a function argument separator (e.g., a comma):
      // Until the token at the top of the stack is a left parenthesis, pop operators
      // off the stack onto the output queue. If no left parentheses are encountered,
      // either the separator was misplaced or parentheses were mismatched.

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
      else if (token == "(") opStack.push(token); // If the token is a left parenthesis (i.e. "("), then push it onto the stack.
      else if (token == ")") { // If the token is a right parenthesis (i.e. ")"):
        // Until the token at the top of the stack is a left parenthesis,
        // pop operators off the stack onto the output queue.
        while (!opStack.empty() && opStack.top() != "(") {EVAL_MOVE_TOP(opStack, q);}

        // Pop the left parenthesis from the stack, but not onto the output queue.
        if (!opStack.empty() && opStack.top() == "(") opStack.pop();
        // (unimplemented) If the token at the top of the stack is a function token, pop it onto the output queue.
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
   @returns ValType
   */
  ValType queue(Queue &q) {
    std::stack<ValType> valStack;

    while (!q.empty()) { // While there are input tokens left, read the next token from input.
      auto &token = EVAL_FPOP(q);

      if (Type::isNumber(token)) valStack.push(std::stod(token)); // If the token is a value, push it on the stack.
      else { // Otherwise, the token is an operator (operator here includes both operators and functions).
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
        else if (token == "^") valStack.push(pow(L, R));
        else if (token == "%") valStack.push(static_cast<int>(L) % static_cast<int>(R));
        else throw EVAL_UNREC_OP;
      }
    }
    if (valStack.size() == 1) return valStack.top(); // If there is only one value in the stack, that value is the result of the calculation.
    else throw EVAL_INPUT_TOO_MANY_VALS; // Otherwise, there are more values in the stack: (Error) The user input has too many values.
  }
}

/**
 Evaluates a string by tokenizing, creating an RPN stack, and evaluating it.
 Note that this is a free function.

 @param[in] str
 @param[in] vars Custom in-place variables (optional)
 @returns result
 */
_eval::ValType eval(std::string str, _eval::ValMap vars = _eval::ValMap()) {
  str.erase(std::remove(std::begin(str), std::end(str), ' '), std::end(str)); // Remove whitespace.
  if (str.empty()) return 0;
  auto q = _eval::read(_eval::rewriteExpression(str), vars);
  return _eval::queue(q);
}
#endif /* eval_h */
