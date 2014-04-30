#include <iostream>
#include <fstream>
using namespace std;

int main() {
    ifstream dtaFile("all.dta");
    ifstream idxFile("all.idx");
    ofstream file1("1.dta");
    ofstream file2("2.dta");
    ofstream file3("3.dta");
    ofstream file4("4.dta");
    ofstream file5("5.dta");
    char allLine[32];

    int index = 0;

    while (idxFile >> index) {
        dtaFile.getline(allLine, 32);
        switch (index) {
            case 1 : file1 << allLine; break;
            case 2 : file2 << allLine; break;
            case 3 : file3 << allLine; break;
            case 4 : file4 << allLine; break;
            case 5 : file5 << allLine; break;
            default : break;
        }
    }
    dtaFile.close();
    idxFile.close();
    file1.close();
    file2.close();
    file3.close();
    file4.close();
    file5.close();
}