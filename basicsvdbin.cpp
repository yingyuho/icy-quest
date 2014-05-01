#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <signal.h>
#include <unistd.h>
#include "record.hpp"

#define USR_NUM 458293
#define MOV_NUM 17770

#define FET_NUM 100

#define USR_LEN (FET_NUM * USR_NUM)
#define MOV_LEN (FET_NUM * MOV_NUM)

#define SIZARR(a) (sizeof(a) / sizeof(*a))

#define LRATE 0.002
#define KREG 0.02

#define KAVG 25

#define REC_BATCH 512

static bool sigint = false;

void sig_handler(int signo) {
    switch (signo) {
    case SIGINT:
        sigint = true;
        break;
    default:
        exit(1);
        break;
    }
}

inline size_t batch_read_record(struct record *r, FILE *f) {
    return fread(r, sizeof(struct record), REC_BATCH, f);
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s trainFile validFile [qualFile]\n", argv[0]);
        return 0;
    }

    fprintf(stderr, "Initializing arrays...\n");

    float *usrFeature = (float *) malloc(sizeof(float) * USR_LEN);
    float *movFeature = (float *) malloc(sizeof(float) * MOV_LEN);

    float *movieMean = (float *) malloc(sizeof(float) * MOV_NUM);
    int *movieNum = (int *) malloc(sizeof(int) * MOV_NUM);

    float *userOffset = (float *) malloc(sizeof(float) * USR_NUM);
    int *userNum = (int *) malloc(sizeof(int) * USR_NUM);

    for (int i = 0; i < USR_LEN; i++)
        usrFeature[i] = 0.1 * float(rand()) / float(RAND_MAX);

    for (int i = 0; i < MOV_LEN; i++)
        movFeature[i] = 0.1 * float(rand()) / float(RAND_MAX);

    FILE *trainFile = fopen(argv[1], "rb");
    FILE *validFile = fopen(argv[2], "rb");

    char *lineBuf = NULL;
    size_t lineLen = 0;
    char *endptr;

    struct record rBuf[REC_BATCH];
    size_t recNum;

    unsigned int ratingSum = 0, ratingNum = 0;
    unsigned int trainEntryNum = 0, validEntryNum = 0;

    fprintf(stderr, "Computing movie rating means...\n");

    while ((recNum = batch_read_record(rBuf, trainFile)))
    for (int ri = 0; ri < recNum; ri++) {
        struct record &rec = rBuf[ri];

        ratingSum += rec.r;
        ++ratingNum;

        movieMean[rec.m] += rec.r;
        ++movieNum[rec.m];
    }
    // printf("%d %f\n", movCount, movieMean[prevID]);
    // movieMean[prevID] /= movieNum[prevID];

    float globalMean = float(ratingSum) / float(ratingNum);

    for (int i = 0; i < MOV_NUM; i++) {
        movieMean[i] = (globalMean * KAVG + movieMean[i]) / (KAVG + movieNum[i]);
        trainEntryNum += movieNum[i];
    }

    // printf("%f\n", globalMean);

    fprintf(stderr, "Computing user rating offsets...\n");

    rewind(trainFile);

    while ((recNum = batch_read_record(rBuf, trainFile)))
    for (int ri = 0; ri < recNum; ri++) {
        struct record &rec = rBuf[ri];
        userOffset[rec.u] += rec.r - movieMean[rec.m];
        ++userNum[rec.u];
    }

    for (int i = 0; i < USR_NUM; i++) {
        userOffset[i] /= float(userNum[i]) + 1E-15;
        // printf("%d %f\n", userNum[i], userOffset[i]);
    }

    fseek(validFile, 0L, SEEK_END);
    validEntryNum = ftell(validFile) / sizeof(struct record);

    if (signal(SIGINT, sig_handler) == SIG_ERR)
        printf("Cannot catch SIGINT\n");

    FILE *usrFile, *movFile;

    // usrFile = fopen("usrfeat.bin", "rb");
    // movFile = fopen("movfeat.bin", "rb");

    // fread(usrFeature, sizeof(float), USR_LEN, usrFile);
    // fread(movFeature, sizeof(float), MOV_LEN, movFile);

    // fclose(usrFile);
    // fclose(movFile);

    for (int t = 0; t < 85; t++) {
        if (sigint)
            break;

        fprintf(stderr, "Pass #%d...\n", t);

        rewind(trainFile);

        double rmse = 0.0;

        while ((recNum = batch_read_record(rBuf, trainFile)))
        for (int ri = 0; ri < recNum; ri++) {
            struct record &rec = rBuf[ri];
            float * const usrRow = usrFeature + FET_NUM * rec.u;
            float * const movRow = movFeature + FET_NUM * rec.m;

            float error = rec.r - (movieMean[rec.m] + userOffset[rec.u]);

            for (int i = 0; i < FET_NUM; i++)
                error -= usrRow[i] * movRow[i];

            rmse += error * error;

            for (int i = 0; i < FET_NUM; i++) {
                float uv = usrRow[i];
                usrRow[i] += LRATE * (error * movRow[i] - KREG * usrRow[i]);
                movRow[i] += LRATE * (error * uv        - KREG * movRow[i]);
            }
        }

        rmse = sqrt(rmse / trainEntryNum);

        fprintf(stderr, "RMSE(Train) = %.4f\n", rmse);

        rmse = 0.0;

        rewind(validFile);

        while ((recNum = batch_read_record(rBuf, validFile)))
        for (int ri = 0; ri < recNum; ri++) {
            struct record &rec = rBuf[ri];
            float * const usrRow = usrFeature + FET_NUM * rec.u;
            float * const movRow = movFeature + FET_NUM * rec.m;

            float error = rec.r - (movieMean[rec.m] + userOffset[rec.u]);

            for (int i = 0; i < FET_NUM; i++)
                error -= usrRow[i] * movRow[i];

            rmse += error * error;
        }

        rmse = sqrt(rmse / validEntryNum);

        fprintf(stderr, "RMSE(Valid) = %.4f\n", rmse);
    }

    // usrFile = fopen("usrfeat.bin", "wb");
    // movFile = fopen("movfeat.bin", "wb");

    // fwrite(usrFeature, sizeof(float), USR_LEN, usrFile);
    // fwrite(movFeature, sizeof(float), MOV_LEN, movFile);

    // fclose(usrFile);
    // fclose(movFile);


    FILE *qualFile = NULL;

    if (argc >= 4) {
        fprintf(stderr, "Predicting qual ratings...\n");

        qualFile = fopen(argv[3], "r");

        while (getline(&lineBuf, &lineLen, qualFile) != -1) {
            unsigned int usrID = strtoul(lineBuf, &endptr, 10) - 1;
            unsigned int movID = strtoul(endptr, &endptr, 10) - 1;
            strtoul(endptr, &endptr, 10);

            float * const usrRow = usrFeature + FET_NUM * usrID;
            float * const movRow = movFeature + FET_NUM * movID;

            float pred = movieMean[movID] + userOffset[usrID];

            for (int i = 0; i < FET_NUM; i++)
                pred += usrRow[i] * movRow[i];

            if (pred < 1.0)
                pred = 1.0;
            if (pred > 5.0)
                pred = 5.0;

            printf("%.3f\n", pred);
        }

        fclose(qualFile);
    }


    free(usrFeature);
    free(movFeature);
    free(lineBuf);

    fclose(trainFile);

    return 0;
}

// F =  40, T = 75: 0.91741 (3.57% above water)
// F = 100, T = 73: 0.91332 (4.00% above water)

