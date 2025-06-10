# myPreCompiler

A modular, robust, and efficient C source code preprocessor written in C.  
`myPreCompiler` simulates core preprocessing stages of a real compiler: it expands `#include` directives, removes comments, validates identifiers, and produces a transformed output with optional detailed statistics and error reports.

---

## Features

- **Recursive `#include` Expansion:** Supports nested and transitive inclusion of files.
- **Comment Removal:** Eliminates both inline (`//`) and multiline (`/* ... */`) comments using regex, preserving line numbering.
- **Identifier Validation:** Checks local and global variable names, logging invalid ones (e.g., illegal characters, starting with digits).
- **Configurable Output:** Writes processed code to a file or stdout, based on CLI options.
- **Verbose Mode:** Prints detailed statistics: removed lines, included files, identifiers checked, errors, and file size/line counts.
- **Robust Error Handling:** Detects and reports invalid CLI parameters, missing or unreadable files, unresolved or recursive includes, write errors, and more.
- **Dynamic Memory Management:** Efficiently processes files of arbitrary size and guarantees no memory leaks.

---

## Architecture

- **main.c** – CLI argument parsing, control flow, orchestration
- **preprocessor.c** – Core logic: includes, comment removal, identifier analysis
- **utils.c** – File handling, line parsing, logging, syntax checks
- **myPreCompiler.h** – Shared data structures, function prototypes, and macros

Each module communicates strictly through the header interface, ensuring low coupling and high cohesion.

---

## Build

To compile the project, run:

```sh
gcc src/main.c src/preprocessor.c src/utils.c -Iinclude -o myPreCompiler.out
