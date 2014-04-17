# include <iostream>
# include <fstream>
# include <ctime>


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
read file and store the data in data[dataPoints][4]
(user number) (movie number) (date number) (rating) 
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
// averageUser[userNumber][3] = [][sum(userRating), userCounts, useOffset]
void averageOperation(int (*data)[4], double (*averageMovie)[3], double (*averageUser)[3]) {
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
    //    averageUser[i][2] = (meanTotalRating * kCoefUser + averageUser[i][0]) / (kCoefUser + averageUser[i][1])
    //        - averageUser[i][0] / averageUser[i][1];
        averageUser[i][2] = (meanTotalRating * kCoefUser + averageUser[i][0]) / (kCoefUser + averageUser[i][1])
            - meanTotalRating;
    }

}


// Simple output the averageMovie[certain movie] + averageUserOffset[certain user]
void readOutputFileTemp(int (*data)[4], double (*averageMovie)[3], double (*averageUser)[3]) {
    ifstream inFile("qual.dta");
    ofstream outFile("1");
    int userQual = 0, movieQual = 0, dataQual = 0;
    while (inFile >> userQual >> movieQual
        >> dataQual) {
        movieQual --;
        userQual --;
        outFile << averageMovie[movieQual][2] + averageUser[userQual][2] << endl;
    }
    inFile.close();
    outFile.close();
}

int main()
{
    unsigned startTime = clock();
    // alocate memory for soring the all.dta
    int (* originalRatingValues)[4] = new int[dataPoints][4];
    if (originalRatingValues == NULL) {
        cout << "Fail: originalRatingValues memory allocation!" << endl;
        exit(0);
    }
    // read all.dta
    readInputFile(originalRatingValues);

    // [sum(movieRating), movieCounts, sum(movieRating ^ 2) -> variance -> newValue]
    double (* averageMovie)[3] = new double[movieNumber][3];
    // [sum(userRating), userCounts, sum(userRating ^ 2) -> variance]
    double (* averageUser)[3] = new double[userNumber][3];

    averageOperation(originalRatingValues, averageMovie, averageUser);

    double (* movieFeature)[featureNumber] = new double[movieNumber][featureNumber];
    double (* userFeature)[featureNumber] = new double[userNumber][featureNumber];

    readOutputFileTemp(originalRatingValues, averageMovie, averageUser);


    // free memory
    delete [] averageUser;
    delete [] averageMovie;
    delete [] originalRatingValues;

    unsigned elapsed = clock() - startTime;
    cout << (double)elapsed / CLOCKS_PER_SEC << endl;
    


    return 0;
}