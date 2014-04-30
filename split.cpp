#include <iostream>
#include <fstream>
#include <cstdio>
using namespace std;

int main() {

    char *allLine = NULL, *idxLine = NULL;
    size_t allLen = 0, idxLen = 0;
    char buff[32];

    FILE *allFile, *idxFile;
    FILE *outFile[5];

    for (int i = 0; i < 5; i++) {
        sprintf(buff, "%d.dta", i+1);
        outFile[i] = fopen(buff, "w");
    }

    allFile = fopen("all.dta", "r");
    idxFile = fopen("all.idx", "r");

    while ((getline(&allLine, &allLen, allFile) != -1) && 
           (getline(&idxLine, &idxLen, idxFile) != -1)) {
        int index = idxLine[0] - '1';
        if ((0 <= index) && (index <= 4))
            fputs(allLine, outFile[index]);
    }

    free(allLine);
    free(idxLine);

    fclose(allFile);
    fclose(idxFile);
    for (int i = 0; i < 5; i++)
        fclose(outFile[i]);
}