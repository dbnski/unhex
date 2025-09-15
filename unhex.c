#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096
#define BIN_SIZE 256

enum State { TEXT, QUOTED_STRING, BINARY, RAW };

const unsigned char modifier[] = "_binary ";

void print_binary(const unsigned char *buf, size_t blen)
{
    printf("0x");
    for (size_t i = 0; i < blen; i++)
    {
        unsigned char c = buf[i];

        switch (c)
        {
            case '\\':
                switch (buf[i+1]) {
                    case '\'':
                    case '\"':
                    case '\\':
                    case '\0':
                        continue;
                    default:
                        fprintf(stderr, "bad sequence: %d%d\n", buf[i], buf[i+1]);
                        exit(1);
                }
        }
        printf("%02x", buf[i]);
    }
}

void print_string(const unsigned char *buf, size_t blen)
{
    putchar('\'');
    fwrite(buf, 1, blen, stdout);
    putchar('\'');
}

void print_raw(const unsigned char *buf, size_t blen)
{
    fwrite(buf, 1, blen, stdout);
}

void print(enum State mode, const unsigned char *buf, size_t blen)
{
    switch (mode)
    {
        case QUOTED_STRING:
            print_string(buf, blen);
            break;
        case RAW:
            print_raw(buf, blen);
            break;
        case BINARY:
            print_binary(buf, blen);
            break;
        default:
            fprintf(stderr, "unsupported mode: %d\n", mode);
            exit(1);
    }
}

int main(void) {
    enum State state = TEXT;
    unsigned char buf[BUF_SIZE];
    unsigned char bin[BIN_SIZE];
    unsigned char *bptr = NULL;
    unsigned char prev = 0;
    size_t blen = 0, mlen = 0, n;
    int in_comment = 0, nl = 1;

    if (BIN_SIZE >= BUF_SIZE)
    {
        fprintf(stderr, "BIN_SIZE must be smaller than BUF_SIZE!\n");
        exit(1);
    }

    while ((n = fread(buf, 1, BUF_SIZE, stdin)) > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
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

                    if (c == modifier[mlen])
                    {
                        mlen++;
                        if (mlen == sizeof(modifier) - 1)
                        {
                            mlen = 0;
                        }
                        continue;
                    }
                    if (mlen > 0)
                    {
                        fwrite(modifier, 1, mlen, stdout);
                        mlen = 0;
                    }

                    if (c == '\'') // opening quote char
                    {
                        state = QUOTED_STRING;
                        bptr = buf + i + 1;
                        blen = 0;
                        continue;
                    }

                    putchar(c);
                    if (c == '\n') {
                        nl = 1;
                    }
                    break;
                case RAW:
                    if (c == '\'' && prev != '\\')
                    {
                        state = TEXT;
                    }
                    putchar(c);
                    break;
                case QUOTED_STRING:
                    if (c < 0x20)
                    {
                        state = BINARY;
                    }
                    /* fallthrough */
                case BINARY:
                    if (c == '\'') // closing quote candidate
                    {
                        if (prev != '\\') // quote char not escaped
                        {
                            print(state, bptr, blen);
                            bptr = NULL;
                            blen = 0;
                            state = TEXT;
                        }
                    }
                    else
                    {
                        blen++;
                        if (blen > BIN_SIZE) // too long
                        {
                            // dump as is, including the leading quote
                            putchar('\'');
                            print(RAW, bptr, blen - 1);
                            putchar(c);

                            bptr = NULL;
                            blen = 0;
                            state = RAW;
                        }
                    }
                    break;
            }

            if (i == n - 1) // end of current buf
            {
                if (state == QUOTED_STRING || state == BINARY)
                {
                    memcpy(bin, bptr, blen);
                    bptr = bin;
                }
            }

            prev = c;
        }
    }

    exit(0);
}