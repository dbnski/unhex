# unhex

A minimal C utility that transforms `_binary '<uuid>'` sequences in input
streams into their hexadecimal representation. Designed for replacing 
binary UUIDs embedded in text files, especially in contexts like MySQL
log rewriting.

## Features

- Detects `_binary '<uuid>'` patterns and replaces them with `0x<hex>` format
- Handles escape sequences (`\\`, `\'`, `\"`, `\0`)

## Notes

- Assumes binary UUIDs are exactly 16 bytes
- Full-line comments are preserved unchanged

## Build

```bash
make
```

## Usage

```bash
$ echo "SELECT * FROM table WHERE id = _binary 'abcdefghijklmnop';" | ./unhex
SELECT * FROM table WHERE id = 0x6162636465666768696a6b6c6d6e6f70;
```
