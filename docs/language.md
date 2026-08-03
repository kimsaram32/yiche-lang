# The YICHE Programming Language

This documentation describes the formal syntax and semantics of the YICHE
Programming Language. Informal descriptions and examples are also included in
blockquotes.

## Conceptual models

### Entities

An *entity* is either a variable or a function.

### Scopes of declarations

A declaration has its associated *scope*, which determines the portion of the
program where the declaration takes effect. A declaration is *visible* at a
point of execution if and only if there is a declaration using the identifier at
that point.

TODO An identifier *resolves* to an entity associated by the nearest visible
declaration.

A declaration has a *global scope* if and only if the scope of such declaration is
from the end of the declaration to the end of the source text.

A declaration has a *block scope* if and only if the scope of such declaration is
from the end of the declaration to the nearest "}".

Two declarations have the same scope if and only if their scopes end at the same
point.

TODO(built-in functions) There cannot exist two declarations with the same
identifier and the same scope,

The scope of a variable declaration is determined as follows:

1. If the declaration declares a function parameter, its scope is the entire
   body of that function.
2. If the declaration occurs inside a function body, it has a block scope.
3. Otherwise, it has a global scope.

A function declaration has a global scope.

## Lexical Structure

If a token can be matched both as a keyword and as an identifier, it is matched
as a keyword. Hence, identifiers cannot have their name as keywords.

### Characters

```ebnf
<visible_character> ::= (? all printable ASCII codes except space (0x20) ?)

<space> ::= " "

<tab> ::= "\t"

<whitespace> ::= <space> | <tab> | "\n" | "\r"

<character> ::= <visible_character> | <whitespace>
```

A whitespace is an ASCII character representing either a space, horizontal tab,
line feed, or carriage return.

A source text consists of printable ASCII characters and whitespaces.

> The language does not really have a concept of line breaks. For example,
> statements cannot be separated by line breaks only; explicit semicolons are
> always required.

### Comments

```ebnf
<single_line_comment> ::= "/" "/" { <character> - "\n" }

<multi_line_comment> ::= "/" "*" { <character> } "*" "/"

<comment> ::= <single_line_comment> | <multi_line_comment>
```

A single-line comment is terminated with a line feed character.

A multi-line comment is always ended with "*/". Therefore, nesting of multi-line
comments is not allowed.

### Operators and punctuation symbols

```ebnf
<operator_and_punctuation> ::= ":" | ";" | ","
                               | "+" | "-" | "*" | "/" | "%"
                               | "<" | ">" | "<=" | ">=" | "==" | "!="
                               | "&&" | "||" | "!"
                               | "="
                               | "(" | ")" | "{" | "}"
```

### Keywords

```ebnf
<keyword> ::= "fn"
              | "if"
              | "int"
              | "return"
              | "var"
              | "while"
```

### Identifiers

```ebnf
<letter> ::= (? A-Z ?) | (? a-z ?) | "_"

<digit> ::= (? 0-9 ?)

<identifier> ::= <letter> { <letter> | <digit> }
```

An identifier begins with a letter, followed by zero or more letters and digits.
A letter is either a lowercase alphabet, an uppercase alphabet, or the the
underscore character ("_").

### Constants

```ebnf
<constant> ::= <integer_constant>
```

#### Integer constant

```ebnf
<integer_constant> ::= <decimal_constant>
                       | <hexadecimal_constant>
                       | <character_constant>

<digit_excluding_zero> ::= <digit> - "0"

<decimal_constant> ::= "0" | <digit_excluding_zero> { <digit> }

<hexadecimal_digit> ::= (? 0-9 ?) | (? A-F ?) | (? a-f ?)

<hexadecimal_constant> ::= "0" ( "X" | "x" ) <hexadecimal_digit> { <hexadecimal_digit> }

<common_single_character> ::= ( <visible_character> - "\\" ) | <space> | <tab>

<common_escape_character> ::= "0"
                              | "a"
                              | "b"
                              | "t"
                              | "n"
                              | "v"
                              | "f"
                              | "r"
                              | "e"
                              | "\\"

<character_constant_single_character> ::= ( <common_single_character> - "'" )

<character_constant_escape_character> ::= <common_escape_character> | "'"

<character_constant_escape_sequence> ::= "\\" <character_constant_escape_character>

<character_constant> ::= "'" ( <character_constant_single_character> | <character_constant_escape_sequence> ) "'"
```

A decimal constant and a hexadecimal constant represent the corresponding
integer values.

If a character constant has an escape sequence, it represents the ASCII-encoded
value of its corresponding character, as given by the following table:

| Escape sequence | Character | ASCII value (decimal) |
|----------------|-----------|------------------------|
| `\0`           | NUL       | 0                      |
| `\a`           | BEL       | 7                      |
| `\b`           | BS        | 8                      |
| `\t`           | HT        | 9                      |
| `\n`           | LF        | 10                     |
| `\v`           | VT        | 11                     |
| `\f`           | FF        | 12                     |
| `\r`           | CR        | 13                     |
| `\e`           | ESC       | 27                     |
| `\\`           | Backslash | 92                     |

Otherwise, it represents the specified character's ASCII-encoded value.

## Expressions

```ebnf
<expression> ::= <assignment_expression>
```

Precedences and associations of operators are described in the grammar. The
subsections are organized in the order of precedence, from highest to lowest.

### Primitive expression

```ebnf
<parenthesized_expression> ::= "(" <expression> ")"

<primitive_expression> ::= <identifier> | <constant> | <parenthesized_expression>
```

When an identifier is used as an expression, it must resolve to a variable. It
is evaluated to the value of that variable.

When a constant is used as an expression, it is evaluated to the value of that
constant.

A parenthesized expression has the same value as its inner expression.

### Function call expression

```ebnf
<function_call_argument_list> ::= <expression> [ "," <function_call_argument_list> ]

<function_call_expression> ::= <primitive_expression>
                               | <identifier> "(" [ <function_call_argument_list> ] ")"
```

The called function must have the same number of parameters as the number of
arguments in the expression.

The arguments are evaluated in left-to-right order, and the function body is executed.

### Prefix expression

```ebnf
<prefix_expression> ::= <function_call_expression>
                       | ( "!" ) <prefix_expression>
```

If `a` is non-zero, then `!a` is evaluated to `0`. Otherwise, it is evaluated to `1`.

### Arithmetic expression

```ebnf
<multiplicative_expression> ::= <prefix_expression>
                                | <multiplicative_expression> ( "*" | "/" | "%" ) <prefix_expression>

<additive_expression> ::= <multiplicative_expression>
                          | <additive_expression> ( "+" | "-" ) <multiplicative_expression>
```

`b` must be non-zero in `/` and `%`.

`a * b` is evaluated to the product of `a` and `b`.

`a / b` is evaluated to the quotient of the division of `a` and `b`, with its
  fractional part discarded.

`a % b` is evaluated to the remainder of the division of `a` and `b`.

`a + b` and `a - b` is evaluated to the sum of and the difference between `a`
and `b`, respectively.

### Comparison expression

```ebnf
<comparison_expression> ::= <additive_expression>
                            | <comparison_expression> ( "<" | ">" | "<=" | ">=" ) <additive_expression>
```

Each comparison operation is evaluated to `1` when the specified relation
between operands is true. Else, it is evaluated to `0`.

### Equality expression

```ebnf
<equality_expression> ::= <comparison_expression>
                          | <equality_expression> ( "==" | "!=" ) <comparison_expression>
```

`a = b` is evaluated to `1` when `a` is equal to `b`, and `0` if they are not equal.

`a != b` is evaluated to the opposite of `a = b`; it is evaluated to `1` if the
operands are not equal, and `0` if they are equal.

### Binary logical expression

```ebnf
<logical_and_expression> ::= <equality_expression>
                             | <logical_and_expression> "&&" <equality_expression>

<logical_or_expression> ::= <logical_and_expression>
                            | <logical_or_expression> "||" <logical_and_expression>
```

> `a && b` and `a || b` are evaluated as usual binary logical operators with
> values `1` and `0`. They have short-circuit behaviors.

`a && b` is evaluated as follows: first, `a` is evaluated. If it equals to `0`, the
evaluation ends with value `0`. Otherwise, `b` is evaluated. If it equals to
`0`, the evaluation ends with value `0`. Else, it is evaluated to `1`.

`a || b` is evaluated as follows: first, `a` is evaluated. If it is not equal to
`0`, the evaluation ends with value `1`. Otherwise, `b` is evaluated. If it is
not equal to `0`, the evaluation ends with value `1`. Else, it is evaluated to
`0`.

### Assignment expression

```ebnf
<assignment_expression> ::= <logical_or_expression>
                            | <identifier> "=" <assignment_expression>
```

The identifier must resolve to a variable. When `a = b` is evaluated, `b` is
first stored to `a`, and the resulting value is `b`.

## Statement

```ebnf
<statement_list> ::= { <statement> }

<statement> ::= <null_statement>
                | <expression_statement>
                | <if_statement>
                | <while_statement>
                | <return_statement>
                | <variable_declaration>
```

### Null statement

```ebnf
<null_statement> ::= ";"
```

A null statement does not perform any actions.

> It is primarily used to express an empty body in other compound statements, for example:
>
> ```yiche
> if (some_condition);
> ```

### Expression statement

```ebnf
<expression_statement> ::= <expression> ";"
```

An expression statement evaluates `<expression>`, discarding its value.

### If statement

```ebnf
<if_statement> ::= "if" "(" <expression> ")" <control_flow_body>
```

An if statement is executed as follows:

1. `<expression>` is evaluated.
2. If its value is not equal to `0`, `<control_flow_body>` is executed.

### While statement

```ebnf
<while_statement> ::= "while" "(" <expression> ")" <control_flow_body>
```

A while statement is executed as follows:

1. `<expression>` is evaluated.
2. If its value is equal to `0`, then the execution is finished.
3. Otherwise, `<control_flow_body>` is executed, and execution goes back to 1.

### Return statement

```ebnf
<return_statement> ::= "return" <expression> ";"
```

A return statement is executed as follows:

1. The execution of current function is terminated.
2. It then returns control to the function call expression where the function is
   called. The function call expression is evaluated to the value of `<expression>`.

## Declaration

```ebnf
<program> ::= { <declaration> }

<declaration> ::= <variable_declaration> | <function_declaration>
```

A program consists of sequences of declarations.

A *declaration* associates an identifier with an entity.

### Variable declaration

```ebnf
<type_specifier> ::= "int"

<initializer> ::= "=" <expression>

<variable_declaration> ::= "var" <identifier> ":" <type_specifier> [ <initializer> ] ";"
```

A variable declaration creates a variable.

If the initializer is absent, the variable is initialized with integer value
zero. Otherwise, the expression in the initializer is evaluated, and the
variable is initialized with the expression's value.

### Function declaration

```ebnf
<function_parameter> ::= "var" <identifier> ":" <type_specifier>

<function_parameter_list> ::= <function_parameter> [ "," <function_parameter_list> ]

<function_declaration> ::= "fn" <identifier> "(" [ <function_parameter_list> ] ")" ":" <type_specifier>
                           "{" <statement_list> "}"
```

A function must return a value by including a return statement in its body.
TODO: this restriction should be changed once the language introduces types.

## Built-in functions

### Printing functions

The printing functions operate on the output stream.

#### The `print` function

```yiche
fn print(int n: int): int
```

The `print` function writes a decimal representation of `n` to the output
stream.

#### The `println` function

```yiche
fn println(int n: int): int
```

The `println` function writes a decimal representation of `n` followed by a new
line character to the output stream.

#### The `print_char` function

```yiche
fn print_char(int n: int): int
```

The `print_char` function writes a ASCII representation of `n` to the output
stream.
