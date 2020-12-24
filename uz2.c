/* uz2 - compress game data for Unreal Tournament 2004 redirection download
 * Copyright 2020 Rob Kendrick <rjek@rjek.com>
 * Licence: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <zlib.h>

#define COMPRESS_BUFSIZ 0x8000

static void print_usage(FILE *out, const char progname[static 1])
{
    fprintf(out, "uz2 - UT2004 data compressor\n"
                 "Copyright 2020 Rob Kendrick <rjekrjek.com>\n"
                 "\n"
                 "usage: %s [-v] [-t] [-o outfile] infile\n"
                 "\n"
                 "\t-v\tverbose mode\n"
                 "\t-t\ttry to compress harder\n"
                 "\t-o\tspecify output file (otherwise infile.uz2)\n",
                 progname
    );
}

struct options {
    bool verbose;
    bool harder;
    const char *infile;
    const char *outfile;
};

static inline size_t write_le_uint32(FILE *f, uint32_t v)
{
    /* TODO: check machine endianness and swap to little endian if needed */
    return fwrite(&v, 1, 4, f);
}

static int compress_file(FILE *infile, FILE *outfile, const struct options opt[static 1])
{
    unsigned char *inbuf;
    size_t readamount = COMPRESS_BUFSIZ;
    size_t bread, inlen;
    size_t tread = 0;
    int ret = 0;

    inbuf = malloc(readamount);

    bread = fread(inbuf, 1, 4, infile);
    if (inbuf[0] != 0xc1 || inbuf[1] != 0x83 || inbuf[2] != 0x2a || inbuf[3] != 0x9e) {
        fprintf(stderr, "%s doesn't look like an Unreal package.\n", opt->outfile);
        ret = -1;
        goto out;
    }

    fseek(infile, 0, SEEK_END);
    inlen = ftell(infile);
    fseek(infile, 0, SEEK_SET);

    while ((bread = fread(inbuf, 1, readamount, infile)) > 0) {      
        unsigned char *outbuf = NULL;
        size_t cout = 0;
        tread += bread;

        cout = compressBound(bread);
        outbuf = malloc(cout);
        compress2(outbuf, &cout, inbuf, bread, opt->harder ? 9 : -1);

        if (opt->verbose) {
            printf("compressed %zu bytes to %zu bytes (%lu%%, %lu%% complete)\n", bread, cout, (cout * 100) / bread, (tread * 100) / inlen);
        }

        write_le_uint32(outfile, cout);
        write_le_uint32(outfile, bread);
        fwrite(outbuf, cout, 1, outfile);
        free(outbuf);
    }

out:
    free(inbuf);
    return ret;
}

int main(int argc, char *argv[])
{
    const char *optfmt = "hvto:";
    int opt;
    extern char *optarg;
    extern int optind, opterr, optopt;
    struct options options = { false, false, NULL, NULL };
    int r;

    while ((opt = getopt(argc, argv, optfmt)) != -1) {
        switch (opt) {
        case 'h':
            print_usage(stdout, argv[0]);
            return EXIT_SUCCESS;
        case 'v':
            options.verbose = true;
            break;
        case 't':
            options.harder = true;
            break;
        case 'o':
            options.outfile = strdup(optarg);
            break;
        default:
            fprintf(stderr, "error: unknown option '%c'\n", opt);
            print_usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }   

    if (optind >= argc) {
        fprintf(stderr, "error: expected filename\n");
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    options.infile = strdup(argv[optind]);

    if (options.outfile == NULL) {
        char *b = malloc(strlen(options.infile) + 5);
        sprintf(b, "%s.uz2", options.infile);
        options.outfile = b;
    }

    FILE *in = fopen(options.infile, "rb");
    if (in == NULL) {
        fprintf(stderr, "error: unable to open '%s': %s\n", options.infile, strerror(errno));
        return EXIT_FAILURE;
    }

    if (truncate(options.outfile, 0) != 0) {
        fprintf(stderr, "error: unable to truncate '%s': %s\n", options.outfile, strerror(errno));
        return EXIT_FAILURE;
    }

    FILE *out = fopen(options.outfile, "wb");
    if (out == NULL) {
        fprintf(stderr, "error: unable to open '%s': %s\n", options.outfile, strerror(errno));
        return EXIT_FAILURE;
    }

    r = compress_file(in, out, &options);

    fclose(out);
    fclose(in);

    free((void *)options.infile);
    free((void *)options.outfile);
    return r == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}