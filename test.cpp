# include <iostream>
# include <fstream>
# include <ctime>
# include <cmath>


# define featureNumber 50 // for training

# define dataPoints 102416306
# define userNumber 458293  // 458293
# define movieNumber 17770  // 17770

# define indexMaxBase 94362233
# define indexMaxValid 96327278
# define indexMaxHidden 98291669
# define indexMaxProbe 99666408
# define indexMaxQual 102416306
/*
indice : number
1 : 94362233    -> [0 : 94362232]
2 : 1965045     -> [94362233 : 96327277]
3 : 1964391     -> [96327278 : 98291668]
4 : 1374739     -> [98291669 : 99666407]
5 : 2749898     -> [99666408 : 102416305]
*/


using namespace std;

/* 
read file and store the data in data[dataPoints][5]
(user number) (movie number) (date number) (rating) (new target modified value = original - baseline) 
[0 : indexMaxBase - 1] set Base
[indexMaxBase : indexMaxValid - 1] set Valid
[indexMaxValid : indexMaxHidden - 1] set Hidden
[indexMaxHidden : indexMaxProbe - 1] set Probe
[indexMaxProbe : indexMaxQual - 1] set Qual
*/
void readInputFile(int (*data)[4]) {
    for (int i = 0; i < dataPoints; i++) {
        for (int j = 0; j < 4; j++) {
            data[i][j] = 0;
        }
    }
    ifstream dtaFile("all.dta");
    ifstream idxFile("all.idx");
    int indexBase = 0,
        indexValid = indexMaxBase,
        indexHidden = indexMaxValid,
        indexProbe = indexMaxHidden,
        indexQual = indexMaxProbe;

    int indice = 0, index = 0;
    
    while (idxFile >> indice) {
        switch (indice) {
            case 5 : index = indexQual; indexQual ++; break;
            case 4 : index = indexProbe; indexProbe ++; break;
            case 3 : index = indexHidden; indexHidden ++; break;
            case 2 : index = indexValid; indexValid ++; break;
            default : index = indexBase; indexBase ++;
        }
        dtaFile >> data[index][0] 
                >> data[index][1] 
                >> data[index][2] 
                >> data[index][3];
    }
    dtaFile.close();
    idxFile.close();
}

// to calculate the mean and variable of the data
// return value:
// averageMovie[movieNumber][3]: [][sum(movieRating), movieCounts, modified mean value]
// averageUser[userNumber][3] = [][sum(userRating), userCounts, userOffset]
// April 22, change to just calculate the modified user mean
// thus averageUser[userNumber][3] = [][sum(userRating), userCounts, modified user mean]
void averageOperation(int (*data)[4], 
                      double (*averageMovie)[3], 
                      double (*averageUser)[3]) {
    // initialize
    for (int i = 0; i < movieNumber; i++) {
        for (int j = 0; j < 3; j++) {
            averageMovie[i][j] = 0;
        }
    }
    for (int i = 0; i < userNumber; i++) {
        for (int j = 0; j < 3; j++) {
            averageUser[i][j] = 0;
        }
    }

    int userIndex = 0, movieIndex = 0; 
    double rating = 0.0, ratingSquare = 0.0;
    double totalRatingSquare = 0.0;
    double totalRating = 0.0;

    for (int i = 0; i < indexMaxBase; i++) {
        userIndex = data[i][0] - 1;     // [0] -> user1
        movieIndex = data[i][1] - 1;    // [0] -> movie1
        rating = (double) data[i][3];
        ratingSquare = rating * rating;
        totalRating += rating;
        totalRatingSquare += ratingSquare;
        averageUser[userIndex][0] += rating;
        averageUser[userIndex][1] ++;
        averageUser[userIndex][2] += ratingSquare;
        averageMovie[movieIndex][0] += rating;
        averageMovie[movieIndex][1] ++;
        averageMovie[movieIndex][2] += ratingSquare;
    }
    // var(x) = E(x^2) - (E(x))^2
    double meanTotalRating = totalRating / indexMaxBase;
    double meanTotalRatingSquare = totalRatingSquare / indexMaxBase;
    double varTotalRating = meanTotalRatingSquare - meanTotalRating * meanTotalRating;

    double meanVarMovieRating = 0.0;
    for (int i = 0; i < movieNumber; i++) {
        // here calculate the var for every single movie;
        if (averageMovie[i][1] != 0 && averageMovie[i][1] != 1) {
            averageMovie[i][2] = averageMovie[i][2] / averageMovie[i][1] 
                - (averageMovie[i][0] * averageMovie[i][0] / (averageMovie[i][1] * averageMovie[i][1]));
        } else {
            averageMovie[i][2] = 0.0;
        }
        meanVarMovieRating += averageMovie[i][2];
    }
    meanVarMovieRating /= movieNumber;
    // here for averageMovie[] = [sum(movieRating), movieCounts, variance]
    double kCoefMovie = meanVarMovieRating / varTotalRating;
    // cout << varTotalRating << endl;
    // cout << kCoefMovie << endl;
    kCoefMovie = 25;
    // store the new value to averageMovie[][2]
    // averageMovie[] = [sum(movieRating), movieCounts, newValue]
    for (int i = 0; i < movieNumber; i++) {
        averageMovie[i][2] = (meanTotalRating * kCoefMovie + averageMovie[i][0]) / (kCoefMovie + averageMovie[i][1]);
    }
    // end for preprocess for movie

    // do the same thing for user
    double meanVarUserRating = 0.0;
    for (int i = 0; i < userNumber; i++) {
        // here calculate the var for every single movie;
        if (averageUser[i][1] != 0 && averageUser[i][1] != 1) {
            averageUser[i][2] = averageUser[i][2] / averageUser[i][1] 
                - (averageUser[i][0] * averageUser[i][0] / (averageUser[i][1] * averageUser[i][1]));
        } else {
            averageUser[i][2] = 0.0;
        }
        meanVarUserRating += averageUser[i][2];
    }
    meanVarUserRating /= userNumber;

    // here for averageUser[] = [sum(userRating), userCounts, variance]
    double kCoefUser = meanVarUserRating / varTotalRating;
    // cout << kCoefUser << endl;
    kCoefUser = 25;
    // store the new value to averageUser[][2] offset
    // averageUser[] = [sum(UserRating), userCounts, Offset]
    for (int i = 0; i < userNumber; i++) {
    // the following are both for different ideas on the offset of user
    //    averageUser[i][2] = (meanTotalRating * kCoefUser + averageUser[i][0]) / (kCoefUser + averageUser[i][1])
    //        - averageUser[i][0] / averageUser[i][1];
    //    averageUser[i][2] = (meanTotalRating * kCoefUser + averageUser[i][0]) / (kCoefUser + averageUser[i][1])
    //        - meanTotalRating;
    // this one is just for modified user mean
        averageUser[i][2] = (meanTotalRating * kCoefUser + averageUser[i][0]) / (kCoefUser + averageUser[i][1]);
    }

}

// For the new target rating matrix, which is the original one - 1/2 (averageMovie[movie] + averageUser[User])
// return the sqrt(dimesion/totalForNewTargetTating) for the initial of features
double newTargetRating(int (*data)[4], 
                      double (*averageMovie)[3], 
                      double (*averageUser)[3],
                      double *newTargetRatingArray) {
    for (int i = 0; i < dataPoints; i++) {
        newTargetRatingArray[i] = 0;
    }

    int movieIndex = 0, userIndex = 0;
    
    double totalForNewTargetRating = 0;

    for (int i = 0; i < indexMaxBase; i++) {
        userIndex = data[i][0] - 1;     // [0] -> user1
        movieIndex = data[i][1] - 1;    // [0] -> movie1
        newTargetRatingArray[i] = (double) data[i][3] - 1 / 2 * (averageMovie[movieIndex][2] + averageUser[userIndex][2]);
        totalForNewTargetRating += newTargetRatingArray[i];
    }
//    cout << totalForNewTargetRating << endl;
    return sqrt(indexMaxBase / totalForNewTargetRating);

}


// return movieFeature[movieIndex] Dot Product userFeature[userIndex]
double sumFeatureDotFeature(int movieIndex,
                            int userIndex, 
                            double (*movieFeature)[featureNumber],
                            double (*userFeature)[featureNumber]) {
    double sum = 0.0;
    for (int i = 0; i < featureNumber; i++) {
        sum += movieFeature[movieIndex][i] * userFeature[userIndex][i];
    }
    return sum;
}

void initializeFeature(double (*featureValue)[featureNumber], int length, double value) {
    for (int i = 0; i < length; i++) {
        for (int j = 0; j < featureNumber; j++) {
            featureValue[i][j] = value;
        }
    }
}

void UVDcalculateNewFeature(double * userFeatureSquare, 
                            double * movieFeatureSquare,
                            double (* movieFeature)[featureNumber],
                            double (* userFeature)[featureNumber],
                            double (* tempMovieFeature)[featureNumber],
                            double (* tempUserFeature)[featureNumber]) {
    for (int i = 0; i < featureNumber; i++) {
        userFeatureSquare[i] = 0;
        movieFeatureSquare[i] = 0;
    }
    for (int i = 0; i < featureNumber; i++) {
        for (int j = 0; j < movieNumber; j++) {
            movieFeatureSquare[i] += movieFeature[j][i] * movieFeature[j][i];
        }
        for (int j = 0; j < userNumber; j++) {
            userFeatureSquare[i] += userFeature[j][i] * userFeature[j][i];
        }
    }

    for (int i = 0; i < featureNumber; i++) {
        for (int j = 0; j < movieNumber; j++) {
            tempMovieFeature[j][i] /= userFeatureSquare[i];
        }
        for (int j = 0; j < userNumber; j++) {
            tempUserFeature[j][i] /= movieFeatureSquare[i];
        }
    }

}

void UVDsimpelLearning(int (* data)[4], 
                        double (* averageMovie)[3], 
                        double (* averageUser)[3], 
                        double (* movieFeature)[featureNumber],
                        double (* userFeature)[featureNumber],
                        double (* tempMovieFeature)[featureNumber],
                        double (* tempUserFeature)[featureNumber],
                        double initialFeatureValue,
                        double (* newTargetRatingArray)) {
    unsigned startTime = clock();

    initializeFeature(movieFeature, movieNumber, initialFeatureValue);
    initializeFeature(userFeature, userNumber, initialFeatureValue);

    initializeFeature(tempMovieFeature, movieNumber, 0);
    initializeFeature(userFeature, userNumber, 0);

    cout << "initializeFeature Done!" << endl;
    unsigned elapsed = clock() - startTime;
    cout << (double)elapsed / CLOCKS_PER_SEC << endl;

    // \Sigma_i(U_{ir}^2) and \Sigma_i(V_{is}^2)
    double * userFeatureSquare = new double[featureNumber];
    double * movieFeatureSquare = new double[featureNumber];

    int userIndex = 0, movieIndex = 0, k = 0;
    double rating;
    for (int i = 0; i < indexMaxBase; i++) {
        if (i % 10000000 == 0) {
            elapsed = clock() - startTime;
            cout << (double)elapsed / CLOCKS_PER_SEC << endl;
        }
        userIndex = data[i][0] - 1;
        movieIndex = data[i][1] - 1;
        rating = newTargetRatingArray[i];
        // following index is for feature
        for (int index = 0; index < featureNumber; index++) {
            rating = rating - sumFeatureDotFeature(movieIndex, userIndex, movieFeature, userFeature) + movieFeature[movieIndex][index] * userFeature[userIndex][index];
            tempUserFeature[userIndex][index] += (movieFeature[movieIndex][index] * rating);
 //           tempMovieFeature[movieIndex][index] += userFeature[userIndex][index] * rating;
        }
    }
    elapsed = clock() - startTime;
    cout << (double)elapsed / CLOCKS_PER_SEC << endl;

    UVDcalculateNewFeature(userFeatureSquare,
                            movieFeatureSquare,
                            movieFeature,
                            userFeature,
                            tempMovieFeature,
                            tempUserFeature);
    elapsed = clock() - startTime;
    cout << (double)elapsed / CLOCKS_PER_SEC << endl;

}


void simpleTraining(int (*data)[4], 
                    double (*averageMovie)[3], 
                    double (*averageUser)[3], 
                    double (*movieFeature)[featureNumber],
                    double (*userFeature)[featureNumber]) {
    for (int i = 0; i < movieNumber; i++) {
        for (int j = 0; j < featureNumber; j++) {
            movieFeature[i][j] = 0.1;
        }
    }

    for (int i = 0; i < featureNumber; i++) {
        for (int j = 0; j < featureNumber; j++) {
            userFeature[i][j] = 0.1;
        }
    }

    // init error that make sum(uF*mF) = rating - ratingBase = 
    double *ratingModified = new double[indexMaxBase];
    for (int i = 0; i < indexMaxBase; i++) {
        int userIndex = data[i][0] - 1;
        int movieIndex = data[i][1] - 1;
        double rating = data[i][3];
        ratingModified[i] = rating - averageMovie[movieIndex][2] + averageUser[userIndex][2];
    }
    int iterationMax = 100, iterationNumber;
    for (int featureIndex = 0; featureIndex < featureNumber; featureIndex++) {
        for (int i = 0; i < indexMaxBase; i++) {
            int userIndex = data[i][0] - 1;
            int movieIndex = data[i][1] - 1;
            double rating = ratingModified[i];
            double err = 0.0, errCompare = 0.0;
            double lrate = 0.001;
            double uf, mf;
            iterationNumber = 0;
            do {
                err = rating - sumFeatureDotFeature(movieIndex, userIndex, movieFeature, userFeature);
                uf = userFeature[featureIndex][userIndex];
                mf = movieFeature[featureIndex][movieIndex];
                userFeature[featureIndex][userIndex] += lrate * err * mf;
                movieFeature[featureIndex][movieIndex] += lrate * err * uf;
                iterationNumber ++;
                errCompare = rating - sumFeatureDotFeature(movieIndex, userIndex, movieFeature, userFeature);
            } while ((abs(err) > abs(errCompare)) && (iterationNumber < iterationMax)); 
            if (abs(err) <= abs(errCompare)) {
                userFeature[featureIndex][userIndex] = uf;
                movieFeature[featureIndex][movieIndex] = mf;
            }
        }
    }
}


// Simple output the averageMovie[certain movie] + averageUserOffset[certain user]
// Now as averageUser been modified to the average instead of the offset, we should 
// just plus the 1/2 of the sum of them
void readOutputFileTemp(int (*data)[4], 
                        double (*averageMovie)[3], 
                        double (*averageUser)[3], 
                        double (*movieFeature)[featureNumber],
                        double (*userFeature)[featureNumber]) {
    ifstream inFile("qual.dta");
    ofstream outFile("21");
    int userQual = 0, movieQual = 0, dataQual = 0;
    double output = 0.0;
    while (inFile >> userQual >> movieQual
        >> dataQual) {
        movieQual --;
        userQual --;
        output = 1 / 2 * averageMovie[movieQual][2] + 
                 1 / 2 * averageUser[userQual][2] + 
                 sumFeatureDotFeature(movieQual, userQual, movieFeature, userFeature);
        outFile << output << endl;
    }
    inFile.close();
    outFile.close();
}

int main()
{
    unsigned startTime = clock();
    // alocate memory for soring the all.dta
    int (* originalRatingValues)[4] = new int[dataPoints][4];
//    if (originalRatingValues == NULL) {
//        cout << "Fail: originalRatingValues memory allocation!" << endl;
//        exit(0);
//    }
    // read all.dta
    readInputFile(originalRatingValues);

    // [sum(movieRating), movieCounts, sum(movieRating ^ 2) -> variance -> newValue]
    double (* averageMovie)[3] = new double[movieNumber][3];
    // [sum(userRating), userCounts, sum(userRating ^ 2) -> variance]
    double (* averageUser)[3] = new double[userNumber][3];

    // Calculate for modified averageMovie and modified averageUser
    averageOperation(originalRatingValues, averageMovie, averageUser);

    // For the new target rating matrix, which is the original one - 1/2 (averageMovie[movie] + averageUser[User])
    double (* newTargetRatingArray) = new double[dataPoints];

    double initialFeatureValue = 0;

    initialFeatureValue = newTargetRating(originalRatingValues, averageMovie, averageUser, newTargetRatingArray);

    double (* movieFeature)[featureNumber] = new double[movieNumber][featureNumber];
    double (* userFeature)[featureNumber] = new double[userNumber][featureNumber];
    // temp matrix for calculation
    double (* tempMovieFeature)[featureNumber] = new double[movieNumber][featureNumber];
    double (* tempUserFeature)[featureNumber] = new double[userNumber][featureNumber];
    
    unsigned elapsed = clock() - startTime;
    cout << (double)elapsed / CLOCKS_PER_SEC << endl;

    UVDsimpelLearning(originalRatingValues, 
                        averageMovie, 
                        averageUser, 
                        movieFeature, 
                        userFeature, 
                        tempMovieFeature, 
                        tempUserFeature, 
                        initialFeatureValue,
                        newTargetRatingArray);


    
    // Simple Training method
//    simpleTraining(originalRatingValues, averageMovie, averageUser, movieFeature, userFeature);

//    readOutputFileTemp(originalRatingValues, averageMovie, averageUser, movieFeature, userFeature);


    // free memory
    delete [] averageUser;
    delete [] averageMovie;
    delete [] originalRatingValues;
    delete [] movieFeature;
    delete [] userFeature;
    delete [] tempMovieFeature;
    delete [] tempUserFeature;
    delete [] newTargetRatingArray;

    elapsed = clock() - startTime;
    cout << (double)elapsed / CLOCKS_PER_SEC << endl;
    


    return 0;
}