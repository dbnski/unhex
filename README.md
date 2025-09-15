# unhex

A minimal C utility that transforms binary strings in MySQL log files
into their hexadecimal representation.

## Features

- Detects `'binstring'` and `_binary 'binstring'` patterns and replaces them with `0x<hex>` format
- Handles escape sequences (`\\`, `\'`, `\"`, `\0`, etc.)

## Notes

- Aborts conversion if a string exceeds 256 bytes leaving the original version (adjustable through defines)
- Full-line comments are preserved unchanged regardless their content

## Build

```bash
make
```

## Usage

```bash
$ echo "SELECT * FROM table WHERE id = _binary 'abcdefghijklmnop';" | ./unhex
SELECT * FROM table WHERE id = 0x6162636465666768696a6b6c6d6e6f70;
```
