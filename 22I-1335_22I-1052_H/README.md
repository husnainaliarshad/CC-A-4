# JSON to Relational CSV Converter

## Overview
This tool converts JSON files into relational CSV tables using Flex for lexical analysis, Yacc/Bison for parsing, and C for AST management and CSV generation, as per Assignment 04 requirements for CS-4031 Compiler Construction, Spring 2025.

## Build Instructions
1. Ensure Flex, Bison, and GCC are installed.
2. Run `make` to compile the program.
   - This generates `json2relcsv` executable.
3. Run `make clean` to remove generated files.

## Usage
```bash
./json2relcsv <input.json> [--print-ast] [--out-dir DIR]
```
- `<input.json>`: Path to the input JSON file.
- `--print-ast`: Optional flag to print the AST to stdout.
- `--out-dir DIR`: Optional flag to specify output directory for CSV files (default: current directory).

## Design Notes
- **Tokenization**: Flex scanner handles JSON tokens with escape sequences in strings and tracks line/column for errors.
- **Parsing**: Yacc parser builds an AST representing the JSON structure.
- **AST**: Defined in `ast.h/c`, with nodes for objects, arrays, and scalars.
- **CSV Generation**: Traverses AST to create tables based on object key sets and array structures, streaming output to files.
- **Error Handling**: Reports first lexical/syntax error with line and column, exits with non-zero status.
- **Memory Management**: All allocated memory (AST, tables) is freed at program end.

## Test Cases
Included in `tests/` directory with expected outputs:

1. **test1.json** (Flat object)
   ```json
   {"id": 1, "name": "Ali", "age": 19}
   ```
   Expected: `main.csv` with `id,name,age`

2. **test2.json** (Array of scalars)
   ```json
   {"movie": "Inception", "genres": ["Action", "Sci-Fi", "Thriller"]}
   ```
   Expected: `main.csv`, `genres.csv`

3. **test3.json** (Array of objects)
   ```json
   {"orderId": 7, "items": [{"sku": "X1", "qty": 2}, {"sku": "Y9", "qty": 1}]}
   ```
   Expected: `main.csv`, `items.csv`

4. **test4.json** (Nested objects)
   ```json
   {"postId": 101, "author": {"uid": "u1", "name": "Sara"}, "comments": [{"uid": "u2", "text": "Nice!"}, {"uid": "u3", "text": "+1"}]}
   ```
   Expected: `main.csv`, `author.csv`, `comments.csv`

5. **test5.json** (Empty structures)
   ```json
   {"data": {}, "list": []}
   ```
   Expected: `main.csv`, `data.csv`, `list.csv`
