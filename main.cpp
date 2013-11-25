#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <unistd.h>
#include "Hotspot.h"
#include "Coordinate.h"

using namespace std;

int width;
int height;
int numberOfRounds;
int currentRound = 0;
bool writeCoords = false;
vector<Coordinate> coords;
vector<Hotspot> hotspots;

vector<double> *oldHeatmap;
vector<double> *currentHeatmap;

string outFile = "output.txt";

int getIndex(int x, int y) {
	return y * width + x;
}

void setHotspots() {
    for (Hotspot h : hotspots) {
        if (currentRound >= h.startRound && currentRound < h.endRound) {
            (*currentHeatmap)[getIndex(h.x, h.y)] = 1;
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

	#pragma omp parallel for reduction(+:sum)
    for (int y = fromY; y < toY + 1; y++) {
    	double innerSum = 0;
    	#pragma omp parallel for reduction(+:innerSum)
        for (int x = fromX; x < toX + 1; x++) {
            innerSum += (*oldHeatmap)[getIndex(x, y)];
        }
        sum += innerSum;
    }

    return sum / 9;
}

void performRound() {
    auto tmp = oldHeatmap;
    oldHeatmap = currentHeatmap;
    currentHeatmap = tmp;
    
    #pragma omp parallel for
    for (int y = 0; y < height; y++) {    
    	#pragma omp parallel for
        for (int x = 0; x < width; x++) {
            (*currentHeatmap)[getIndex(x,y)] = getAverageHeat(x, y);
        }
    }

    currentRound++;
    setHotspots();
}

// have to use *& to pass pointer as reference, otherwise we get a copy
// of the pointer, and if we assign a new memory location to it this is local

void initializeHeatmap(vector<double> *&heatmap) {
	int size = width * height;
    heatmap = new vector<double>(size);
    #pragma omp parallel for
    for (int y=0; y<height; y++) {
    	#pragma omp parallel for
    	for (int x = 0; x<width; x++) {
        	(*heatmap)[getIndex(x, y)] = 0;
    	}
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
        for (int y=0; y<height; y++) {
           	for (int x=0; x<width; x++) {
           		double cell = (*currentHeatmap)[getIndex(x, y)];
                output << getOutputValue(cell).c_str();
            }
            output << "\n";
        }
    } else {
        for (Coordinate coord : coords) {
           	double cell = (*currentHeatmap)[getIndex(coord.x, coord.y)];
            output << cell <<"\n";
        }
    }
    output.close();
}

int main(int argc, char* argv[]) {
    //read input
    width = atoi(argv[1]);
    height = atoi(argv[2]);
    numberOfRounds = atoi(argv[3]);

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

    while (currentRound < numberOfRounds) {
        performRound();
    }

    writeOutput();

    exit(0);
}
