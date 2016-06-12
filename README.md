# libdetection

Extendable library for detection syntaxes by formal notations.
Can be used to detect injections and commanding attacks such as SQLi and others.
Does not require attacks samples to learn.


## Requirements

- cmake
- cunit
- bison
- re2c
- [libavl2](http://git.fruit.je/avl)

## Installation

To build, run

```sh
$ ./configure
```
or, to build with optimizations

```sh
$ CFLAGS="-Ofast -mtune=native -march=native" ./configure
```

then run make

```sh
$ make -C build
```

After that, you will get

1. The library `build/lib/libdetection.so`
2. Example executable: `build/perf/perf`


## Extend syntaxes

To add your own module to the library, you have to

1. Add the module code into a new directory under `lib/`. See
   `lib/sqli/sqli.c` code which implements libdetection interface
   for sqli module (look at `detect_parser_sqli` global variable).
2. Make libdetection to load the module statically, see
   `TRYLOAD` in `detect_parser_init()`. Dynamic loading is not
   implemented yet.


## Formal model

For attacks detection, a formal model is used which allows to make a decision based on the type of attack. This approach allows us to implement the library without having to specify precedents of attacks (without a signature of each specific attack). Here you will not find the files with fingerprints or static rules with regular expressions.

Determination of the attack - the user input (string coming to the input of the library) can be processed as a sequence of data, among which there will be at least one syntax instruction.

For example, in the string `123 union select` there are one data token and two instructions (commands). And the string is considered to be an attack. But it’s not complete as there no names of column and table used.

Within the library, each parser state (where line processing begins) is called a context. Contexts are formulated in such a way to reduce the number of false positives of similar type.


### Types of attacks in a single syntax

The library supports a variety of contexts (the parser states) with two main groups - injection attacks and commanding attacks. This approach allows us to share all the attacks on the two groups suitable for subsequent analysis and attribution to vulnerabilities in the code.

For example, for phpMyAdmin or similar tools, a set of parameters for query parameter will take attacks such commanding as legitimate. Such attacks will consist entirely of SQL syntax for initial parsing state: `? Query = SELECT id FROM users ...`. At the same time the defective syntax of SQL (injection attacks) in these parameters can be easily blocked, for example: `? Query = 123 UNION SELECT if FROM users - a-`.

For ease of understanding, you can distinguish injections and commanding attacks in the following way. In the case of injection attacks, there is, at least, one user input to be handled entirely as data. If the case of commanding attacks any user input will contain at least one instruction.


### Syntax

Currently, we publish PoC for only SQL syntax. This allows you to test the idea against different SQL-injection attacks. To add new syntax (eg, path traversal, bash / sh, PHP, HTML5, JavaScript) you only need to describe the lexer and the BNF. So you can avoid programming in C. Just copy directory `./lib/sqli`, and then edit the files `sqli_lexer.re2c` (description lexer) and `sqli_parser.y` (syntax description in BNF). That's all you need to do. No need to write regular expressions or collecting attacks samples.
