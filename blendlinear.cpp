#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <Accelerate/Accelerate.h>
#include "record.hpp"

#define PROB_LEN 1374739
#define QUAL_LEN 2749898

using namespace std;

int main (int argc, char *argv[]) {
    size_t blendNum = 0;
    vector<float> probePred, a;
    vector<float> qualPred;
    vector<float> probeRate(PROB_LEN, 0.0);
    vector<float> weight;
    vector<float> result(QUAL_LEN, 0.0);

    char *lineBuf = NULL;
    char buf[128];
    size_t lineLen = 0;
    char *endptr, *token;
    size_t count;

    FILE *f;

    strcpy(buf, "proberate.dta");
    fprintf(stderr, "Load %s\n", buf);

    f = fopen(buf, "r");

    count = 0;

    while (getline(&lineBuf, &lineLen, f) != -1) {
        probeRate[count++] = strtof(lineBuf, NULL);
    }

    fclose(f);

    float * fPtr;

    for (count = 1; count < argc; count++) {
        token = strrchr(argv[count], '/');
        if (token && *(token + 1) == '\0')
            *token = '\0';

        sprintf(buf, "%s/probe.dta", argv[count]);
        fprintf(stderr, "Load %s\n", buf);

        f = fopen(buf, "r");

        probePred.resize(PROB_LEN * (blendNum + 1), 0.0);

        fPtr = (float *) &probePred[PROB_LEN * blendNum];

        while (getline(&lineBuf, &lineLen, f) != -1) {
            *(fPtr++) = strtof(lineBuf, NULL);
        }

        fclose(f);

        sprintf(buf, "%s/qual.dta", argv[count]);
        fprintf(stderr, "Load %s\n", buf);

        f = fopen(buf, "r");

        qualPred.resize(QUAL_LEN * (blendNum + 1), 0.0);

        fPtr = (float *) &qualPred[QUAL_LEN * blendNum];

        while (getline(&lineBuf, &lineLen, f) != -1) {
            *(fPtr++) = strtof(lineBuf, NULL);
        }

        fclose(f);

        ++blendNum;
    }

    __CLPK_integer info, m, n, lda, ldb, nrhs;

    m = PROB_LEN;
    n = blendNum;
    lda = PROB_LEN;
    ldb = PROB_LEN;
    nrhs = 1;

    __CLPK_integer lwork = m + 2 * n;
    vector<__CLPK_real> work(lwork);

    char transa[] = "N";

    weight = probeRate;
    a = probePred;

    sgels_(transa, &m, &n, &nrhs, 
        &a[0], &lda, &weight[0], &ldb, 
        &work[0], &lwork, &info);

    a.clear();

    weight.resize(blendNum);

    count = 0;

    for (int i = 0; i < blendNum; i++) {
        for (int j = 0; j < QUAL_LEN; j++) {
            result[j] += weight[i] * qualPred[count++];
        }
    }

    double rmse = 0.0;

    for (int j = 0; j < PROB_LEN; j++) {
        for (int i = 0; i < blendNum; i++) {
            probeRate[j] -= weight[i] * probePred[i * PROB_LEN + j];
        }
        rmse += probeRate[j] * probeRate[j];
    }

    rmse = sqrt(rmse / PROB_LEN);

    fprintf(stderr, "RMSE(Probe) = %.4f\n", rmse);

    for (int j = 0; j < QUAL_LEN; j++) {
        printf("%.3f\n", result[j]);
    }

    free(lineBuf);

    return 0;
}