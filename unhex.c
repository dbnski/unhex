#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define BIN_LEN  16   // our binary is 16 bytes

enum State { TEXT, BINARY, ESCAPE };

const char prefix[] = "_binary '";

void print_binary(const unsigned char *bin, size_t blen)
{
    printf("0x");
    for (size_t i = 0; i < blen; i++)
        printf("%02x", bin[i]);
}

int main(void) {
    enum State state = TEXT;
    unsigned char buf[BUF_SIZE];
    unsigned char bin[BIN_LEN];
    size_t blen = 0, n;
    int in_comment = 0, nl = 1;
    int mlen = 0;

    while ((n = fread(buf, 1, BUF_SIZE, stdin)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char c = buf[i];

            switch (state)
            {
                case TEXT:
                    // at the start of a line
                    if (nl) {
                        // starts full-line comment
                        if  (c == '#') {
                            in_comment = 1;
                        }
                        nl = 0;
                    }

                    if (in_comment) {
                        putchar(c);
                        if (c == '\n') {
                            in_comment = 0;
                            nl = 1;
                        }
                        continue;
                    }

                    // try matching the prefix
                    if (c == prefix[mlen]) {
                        mlen++;
                        // full prefix found
                        if (mlen == sizeof(prefix) - 1) {
                            state = BINARY;
                            blen = 0;
                            mlen = 0;
                        }
                        continue;
                    }

                    // write matching chars we've eaten to this point
                    if (mlen > 0) {
                        fwrite(prefix, 1, mlen, stdout);
                        mlen = 0;
                    }

                    putchar(c);
                    if (c == '\n') {
                        nl = 1;
                    }
                    break;
                case ESCAPE:
                    // test for known escaped characters
                    switch (c)
                    {
                        case '"':
                        case '\'':
                        case '\\':
                        case '\0':
                            if (blen == BIN_LEN)
                            {
                                fprintf(stderr, "binary value buffer overflow\n");
                                exit(1);
                            }
                            bin[blen++] = c;
                            break;
                        default: // not one of the escaped characters
                            if (blen + 1 == BIN_LEN)
                            {
                                fprintf(stderr, "binary value buffer overflow\n");
                                exit(1);
                            }
                            bin[blen++] = '\\'; // recover non-escaping backslash
                            bin[blen++] = c;
                            break;
                    }
                    state = BINARY;
                    break;
                case BINARY:
                    switch (c)
                    {
                        case '\'': // ends binary string
                            print_binary(bin, blen);
                            state = TEXT;
                            break;
                        case '\\':
                            state = ESCAPE;
                            break;
                        default:
                            if (blen == BIN_LEN)
                            {
                                fprintf(stderr, "binary value buffer overflow\n");
                                exit(1);
                            }
                            bin[blen++] = c;
                            break;
                    }
                    break;
            }
        }
    }

    if (state == BINARY && blen > 0) {
        print_binary(bin, blen);
        fprintf(stderr, "warning: incomplete binary sequence\n");
    }

    exit(0);
}
