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
                if (i == blen - 1) {
                    fprintf(stderr, "incomplete escape sequence\n");
                    exit(1);
                }
                switch (buf[i+1]) {
                    case '\'':
                    case '"':
                    case '\\':
                    case '\0':
                        c = buf[++i];
                        break;
                    case '0':
                        c = 0x00;
                        i++;
                        break;
                    case 'b':
                        c = 0x08;
                        i++;
                        break;
                    case 'n':
                        c = 0x0a;
                        i++;
                        break;
                    case 'r':
                        c = 0x0d;
                        i++;
                        break;
                    case 't':
                        c = 0x09;
                        i++;
                        break;
                    case 'Z':
                        c = 0x1a;
                        i++;
                        break;
                    default:
                        fprintf(stderr, "bad sequence: %02x %02x\n", c, buf[i+1]);
                        exit(1);
                }
        }
        printf("%02x", c);
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
    size_t blen = 0, mlen = 0, n;
    int in_comment = 0, nl = 1, bs = 0, in_binary = 0;

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
                        break;
                    }

                    if (c == modifier[mlen])
                    {
                        mlen++;
                        if (mlen == sizeof(modifier) - 1)
                        {
                            in_binary = 1;
                            mlen = 0;
                        }
                        break;
                    }
                    if (mlen > 0)
                    {
                        fwrite(modifier, 1, mlen, stdout);
                        mlen = 0;
                    }

                    if (c == '\'') // opening quote char
                    {
                        state = in_binary ? BINARY : QUOTED_STRING;
                        in_binary = 0;
                        bptr = buf + i + 1;
                        blen = 0;
                        break;
                    }

                    putchar(c);
                    if (c == '\n') {
                        nl = 1;
                    }
                    break;
                case RAW:
                    if (c == '\\')
                    {
                        bs++;
                    }
                    else
                    {
                        if (c == '\'' && bs % 2 == 0)
                        {
                            state = TEXT;
                        }
                        bs = 0;
                    }

                    putchar(c);
                    break;
                case QUOTED_STRING:
                    if (c < 0x20 || c == 0x7f)
                    {
                        state = BINARY;
                    }
                    /* fallthrough */
                case BINARY:
                    if (c == '\\')
                    {
                        bs++;
                    }
                    else
                    {
                        if (c == '\'' && bs % 2 == 0)
                        {
                            print(state, bptr, blen);
                            bptr = NULL;
                            blen = 0;
                            state = TEXT;
                            break; // we're done in this branch
                        }
                        bs = 0;
                    }

                    blen++;
                    if (blen > BIN_SIZE) // string too long
                    {
                        // dump as is including the leading quote
                        putchar('\'');
                        print(RAW, bptr, blen - 1);
                        putchar(c);

                        bptr = NULL;
                        blen = 0;
                        state = RAW;
                    }
                    if (bptr) // is context buffer in use
                    {
                        bptr[blen - 1] = c; // maintain the context buffer
                    }
                    break;
            }

            if (i == n - 1) // end of current read buf
            {
                if (state == QUOTED_STRING || state == BINARY)
                {
                    // use the carry over context buffer
                    memcpy(bin, bptr, blen);
                    bptr = bin;
                }
            }
        }
    }

    exit(0);
}
