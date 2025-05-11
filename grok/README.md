# JSON to Relational CSV Converter

## Overview
This tool converts JSON files to relational CSV tables using Flex for tokenization, Yacc/Bison for parsing, and C for AST processing and CSV generation. It follows the conversion rules (R1–R6) specified in the assignment, handling nested JSON up to 30 MiB, assigning primary and foreign keys, and streaming CSV output.

## Build Instructions
1. Ensure Flex, Bison, and GCC are installed.
2. Run `make` to build the `json2relcsv` executable.
3. Run the tool:
   ```bash
   ./json2relcsv < input.json [--print-ast] [--out-dir DIR]
   ```
   - `--print-ast`: Prints the AST to stdout.
   - `--out-dir DIR`: Specifies the output directory for CSV files (default: current directory).

## Design Notes
- **Lexing**: `scanner.l` tokenizes JSON (punctuation, strings, numbers, keywords), tracks line/column for errors, and supports Unicode escapes.
- **Parsing**: `parser.y` defines JSON grammar and builds an AST using `ast.h` structures.
- **Semantic Analysis**: `analyze_ast` in `ast.c` implements conversion rules:
  - R1: Objects with same keys → one table.
  - R2: Array of objects → child table with FK.
  - R3: Array of scalars → junction table.
  - R4: Scalars → columns, null → empty.
  - R5: Assign `id` and `<parent>_id` FKs.
  - R6: One CSV file per table with headers.
- **CSV Generation**: `generate_csv` streams output to handle large files, quoting strings and handling commas.
- **Error Handling**: Reports first lexical/syntax error with line/column; checks file/memory errors.
- **Memory Safety**: Valgrind-clean with `free_ast` and `free_tables` to prevent leaks.

## Test Cases
Located in `tests/`:
1. `test1.json`: Flat object (Example 1).
2. `test2.json`: Array of scalars (Example 2).
3. `test3.json`: Array of objects (Example 3).
4. `test4.json`: Nested objects + reused shape (Example 4).
5. `test5.json`: Complex nested JSON with mixed types.

Each test includes the input JSON and expected CSV outputs in `tests/expected/`.

## Usage Example
```bash
./json2relcsv < tests/test4.json --out-dir output --print-ast
```
Outputs AST to stdout and CSV files (`posts.csv`, `users.csv`, `comments.csv`) in `output/`.

## Limitations
- Assumes valid JSON input (reports first error and exits).
- Table names are derived from field names or "root"; may need sanitization for complex cases.
- Large inputs are streamed, but memory usage depends on AST size.