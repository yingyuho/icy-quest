#include <cstdio>
#include <cstdlib>
#include "record.hpp"

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s inFile outFile\n", argv[0]);
        return 0;
    }

    char *lineBuf = NULL;
    size_t lineLen = 0;

    FILE *inFile = fopen(argv[1], "r");
    FILE *outFile = fopen(argv[2], "wb");

    struct record rec;

    // unsigned int temp;
    char *endptr;

    while (getline(&lineBuf, &lineLen, inFile) != -1) {
        rec.u = strtoul(lineBuf, &endptr, 10) - 1;
        rec.m = strtoul(endptr, &endptr, 10) - 1;
        rec.d = strtoul(endptr, &endptr, 10);
        rec.r = strtoul(endptr, &endptr, 10);

        fwrite(&rec, sizeof(struct record), 1, outFile);
    }

    free(lineBuf);

    fclose(inFile);
    fclose(outFile);

    return 0;
}