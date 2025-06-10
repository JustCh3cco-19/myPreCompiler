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
```

---

## Usage
```sh
./myPreCompiler.out -i <input_file> [-o <output_file>] [-v]
```
**Options:**
- `-i <input_file>`: Input C source file to preprocess
- `-o <output_file>`: Output file (optional, defaults to stdout)
- `-v`: Verbose mode - prints detailed statistics

**Examples:**
# Basic preprocessing
```sh
./myPreCompiler.out -i source.c -o processed.c
```
# Verbose output to stdout
```sh
./myPreCompiler.out -i source.c -v
```
# Process with statistics
```sh
./myPreCompiler.out -i main.c -o main_processed.c -v
```
---

## Processing Pipeline

1. **Argument Parsing:** Recognition and validation of CLI options (`--in`, `--out`, `--verbose`)
2. **Include Expansion:** Recursive inclusion of file contents, assumed to be in the working directory
3. **Comment Removal:** Elimination of inline (`//`) and multiline (`/* */`) comments via regex, maintaining original line numbering
4. **Identifier Validation:** Lexical checking of local and global declarations, logging invalid identifiers
5. **Output Generation:** Writing transformed code to file or stdout based on options
6. **Statistics (Optional):** In verbose mode, tracks: removed lines, included files, checked variables, identified errors, input/output file size and line counts

---

## Testing

Navigate to the `test` directory and run the test cases:
```sh
cd test
```
# Test 1: Comment removal
```sh
../myPreCompiler.out -i test_comments.c -v -o test_comments_processed.c
```
# Test 2: Identifier validation
```sh
../myPreCompiler.out -i test_identifiers.c -v -o test_identifiers_processed.c
```
# Test 3: Include expansion
```sh
../myPreCompiler.out -i test_include_main.c -v -o test_include_processed.c
```
# Test 4: Malformed include handling
```sh
../myPreCompiler.out -i test_malformed_include.c -v -o test_malformed_processed.c
```
# Test 5: General preprocessing
```sh
../myPreCompiler.out -i test2.c -v -o test2_processed.c
```
# Verify output integrity
```sh
diff original_file.c processed_file.c
```
---

## Data Structures and Memory Management

- **Dynamic Allocation:** Text I/O functions use dynamic allocation to handle files of arbitrary size
- **Resizable Buffers:** Strings are read line-by-line using resizable buffers
- **Dynamic Arrays:** Lists of errors, included files, and analyzed variables are managed via dynamic arrays
- **LIFO Deallocation:** Memory is explicitly deallocated at the end of processing or on fatal errors using LIFO scheme, ensuring no memory leaks

---

## Error Handling

The system handles:
- CLI syntax errors
- Non-existent or unreadable files
- Unresolved or recursive includes
- Corrupted or empty files
- Output write errors

All errors are redirected to `stderr` and the program exits with `EXIT_FAILURE`.

---

## Validation

The system has been validated through comprehensive test cases:
- **test_comments.c:** Verifies correct removal of single and multiline comments
- **test_identifiers.c:** Identifies variables with invalid syntax
- **test_include_main.c, test_include_header.h:** Direct and transitive inclusion
- **test_malformed_include.c:** Robust handling of malformed or missing include directives

Output is compared with originals using `diff`, and statistics are inspected for consistency.
