#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include "Hotspot.h"
#include "Coordinate.h"

using namespace std;

int blockSideLengthPerThread = 10;

struct Rectangle {
    int fromX;
    int toX;
    int fromY;
    int toY;
};

int width;
int height;
int noRounds;
int currentRound = 0;
bool writeCoords = false;
vector<Coordinate> coords;
vector<Hotspot> hotspots;

vector<vector<double >> *oldHeatmap;
vector<vector<double >> *currentHeatmap;

string outFile = "output.txt";

void setHotspots() {
    for (Hotspot h : hotspots) {
        if (currentRound >= h.startRound && currentRound < h.endRound) {
            (*currentHeatmap)[h.y][h.x] = 1;
        }
    }
}

vector<vector<int >> parseCsv(string fileName) {
    ifstream file(fileName.c_str());
    vector < vector<int >> result;
    string line;
    //ignore first line
    getline(file, line);
    while (getline(file, line)) {
        vector<int> items;
        stringstream ss(line);
        int i;
        while (ss >> i) {
            items.push_back(i);
            if (ss.peek() == ',') ss.ignore();
        }
        result.push_back(items);
    }
    file.close();
    return result;
}

double getAverageHeat(int centerX, int centerY) {
    int fromX = centerX == 0 ? 0 : centerX - 1;
    int toX = centerX == width - 1 ? centerX : centerX + 1;
    int fromY = centerY == 0 ? 0 : centerY - 1;
    int toY = centerY == height - 1 ? centerY : centerY + 1;

    double sum = 0;

    for (int y = fromY; y < toY + 1; y++) {
        vector<double> *row = &(*oldHeatmap)[y];
        for (int x = fromX; x < toX + 1; x++) {
            sum += (*row)[x];
        }
    }

    return sum / 9;
}

void *calcHeatValues(void *args) {
    Rectangle *rect = (Rectangle *) args;
    for (int y = rect->fromY; y < rect->toY + 1; y++) {
        vector<double> *row = &(*currentHeatmap)[y];
        for (int x = rect->fromX; x < rect->toX + 1; x++) {
            (*row)[x] = getAverageHeat(x, y);
        }
    }
    return NULL;
}

void performRound() {
    auto tmp = oldHeatmap;
    oldHeatmap = currentHeatmap;
    currentHeatmap = tmp;

    vector<pthread_t> threads;

    for (int j = 0; j < height; j += blockSideLengthPerThread) {
        for (int i = 0; i < width; i += blockSideLengthPerThread) {
            Rectangle *rect = new Rectangle;
            rect->fromX = i;
            rect->fromY = j;
            rect->toX = i + blockSideLengthPerThread - 1;
            rect->toY = j + blockSideLengthPerThread - 1;
            if (rect->toX > width - 1) rect->toX = width - 1;
            if (rect->toY > height - 1) rect->toY = height - 1;

            pthread_t thread;
            pthread_create(&thread, NULL, calcHeatValues, rect);
            threads.push_back(thread);
        }
    }

    for (pthread_t thread : threads) {
        pthread_join(thread, NULL);
    }

    setHotspots();
    currentRound++;
}

// have to use *& to pass pointer as reference, otherwise we get a copy
// of the pointer, and if we assign a new memory location to it this is local

void initializeHeatmap(vector<vector<double >> *&heatmap) {
    heatmap = new vector < vector<double >> ();
    for (int y = 0; y < height; y++) {
        vector<double> row;
        for (int x = 0; x < width; x++) {
            row.push_back(0);
        }
        heatmap->push_back(row);
    }
}

string getOutputValue(double cell) {
    string value;
    if (cell > 0.9) {
        value = "X";
    } else {
        value = to_string((int) floor((cell + 0.09) * 10));
    }
    return value;
}

void writeOutput() {
    remove(outFile.c_str());
    ofstream output(outFile.c_str());
    if (!writeCoords) {
        for (vector<double> row : *currentHeatmap) {
            for (double cell : row) {
                output << getOutputValue(cell).c_str();
            }
            output << "\n";
        }
    } else {
        for (Coordinate coord : coords) {
            double cell = (*currentHeatmap)[coord.y][coord.x];
            output << cell <<"\n";
        }
    }
    output.close();
}

void printHeatmap(vector<vector<double >> *heatmap) {
    for (vector<double> row : *heatmap) {
        for (double cell : row) {
            printf("%s ", getOutputValue(cell).c_str());
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    //read input
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    noRounds = atoi(argv[3]);

    string hotspotsFile = string(argv[4]);

    if (argc == 6) {
        string coordFile = string(argv[5]);
        for (vector<int> line : parseCsv(coordFile)) {
            Coordinate coord(line.at(0), line.at(1));
            coords.push_back(coord);
        }
        writeCoords = true;
    }

    //read hotspots
    for (vector<int> line : parseCsv(hotspotsFile)) {
        Hotspot hotspot(line.at(0), line.at(1), line.at(2), line.at(3));
        hotspots.push_back(hotspot);
    }

    initializeHeatmap(currentHeatmap);
    initializeHeatmap(oldHeatmap);
    setHotspots();

    while (currentRound < noRounds + 1) {
        performRound();
    }

    writeOutput();

    exit(0);
}
