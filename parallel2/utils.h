/**
 *
 * Ant Colony Traveling Salesman Problem Optimization - MPI
 * utils.h - help functions
 * Marc Schaer
 *
 **/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <sys/time.h>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

#define INFTY 999999999

double start, end;

int getMatrixIndex(int i, int j, int matrixSize) {
  return (i * matrixSize) + j;
}

void printPath(int* path, int nCities) {
  int i;
  printf("Path : ");
  for (i = 0; i < nCities; i++) {
    if (i == 0) {
      printf("%d", path[i]);
    } else {
      printf(" -> %d", path[i]);
    }
  }
  printf("\n");
}

void printMap(int* map, int nCities) {
  printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
  printf("MAP :\n");
  int i;
  for (i = 0; i < nCities * nCities; i++) {
    printf("%d ", map[i]);
    if (i % nCities == (nCities - 1)) {
      printf("\n");
    }
  }
  printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
}

void copyVectorInt(int* in, int* out, int size) {
  int k;
  for (k = 0; k < size; k++) {
    out[k] = (int) in[k];
  }
}

void copyVectordouble(double* in, double* out, int size) {
  int k;
  for (k = 0; k < size; k++) {
    out[k] = (double) in[k];
  }
}

/**
 * returns the pheromons value of a given path
 *
 **/
void findPheromonsPath (double* pheromonsPath, int* bestPath, double* pheromons, int nCities) {
  int i;
  int previousCity = 0;
  int nextCity = 0;
  for (i = 1; i < nCities; i++) {
    previousCity = bestPath[i - 1];
    nextCity = bestPath[i];
    pheromonsPath[i-1] = pheromons[getMatrixIndex(previousCity, nextCity, nCities)];
  }
  // Add end of loop
  pheromonsPath[nCities - 1] = pheromons[getMatrixIndex(nextCity, bestPath[0], nCities)];
}

/**
 * Load map from a file given by generate_map.cpp code
 * Returns 0 if everything is fine
 **/
int LoadCities(char* file, int* map) {
  std::ifstream in;
  int matrixFull = 1;
  int size = 0;
  int x = 0;
  int y = 0;
  int first = 1;

  in.open (file);

  if (!in.is_open()) {
    printf("Cannot open file.\n");
    return -1;
  }
  char out[8];
  while (!in.eof() && matrixFull) {
    if (first) {
      first = 0;
      in >> out;
      size = atoi(out);
    } else {
      in >> out;
      int index = getMatrixIndex(x,y,size);
      map[index]= atoi(out);
      y = (y + 1) % size;
      if (y == 0) {
        x = (x + 1) % size;
        if (x == 0) {
          matrixFull = 0;
        }
      }
    }
  }

  in.close();

  return 0;
}

/**
 * Compute the probability to go in each city from current city
 **/
void computeProbabilities(int currentCity, double* probabilities, int* path, int* map, int nCities, double* pheromons, double alpha, double beta) {
  int i;
  double total = 0;
  for (i = 0; i < nCities; i++) {
    if (path[i] != -1 || i == currentCity) {
      probabilities[i] = 0.0;
    } else {
      double p = pow(1.0 / map[getMatrixIndex(currentCity,i,nCities)],alpha) * pow(pheromons[getMatrixIndex(currentCity,i,nCities)], beta);
      probabilities[i] = p;
      total += p;
    }
  }

  // If all the probabilities are really small
  // We select one (not randomly to have always the same behavior)
  if (total == 0) {
    i = 0;
    for (i = 0; i < nCities; i++) {
      if (path[i] == -1 && i != currentCity) {
        probabilities[i] = 1.0;
        total++;
      }
    }
  }

  for (i = 0; i < nCities; i++) {
    probabilities[i] = probabilities[i] / total;
  }
}

/**
 * Given the current city, select the next city to go to (for an ant)
 **/
int computeNextCity(int currentCity, int* path, int* map, int nCities, double* pheromons, double alpha, double beta, long random) {
  int i = 0;
  double *probabilities;
  probabilities = (double*) malloc(nCities*sizeof(double));
  computeProbabilities(currentCity, probabilities, path, map, nCities, pheromons, alpha, beta);

  int value = (random % 100) + 1;
  int sum = 0;

  for (i = 0; i < nCities; i++) {
    sum += ceilf(probabilities[i] * 100);
    if (sum >= value) {
      free(probabilities);
      return i;
    }
  }
  free(probabilities);
  return -1;
}

/**
 * Compute the cost of the new path and returns the best cost between the current best and the one
 * from the new path.
 **/
long computeCost(long bestCost, int* bestPath, int* currentPath, int* map, int nCities) {
  // compute currentCost
  int i;
  long currentCost = 0;
  int* orderedCities = (int*) malloc(nCities*sizeof(int));

  for (i = 0; i < nCities; i++) {
    orderedCities[currentPath[i]] = i;
  }
  for (i = 0; i < nCities - 1; i++) {
    currentCost += map[getMatrixIndex(orderedCities[i],orderedCities[i + 1], nCities)];
  }
  // add last
  currentCost += map[getMatrixIndex(orderedCities[nCities - 1], orderedCities[0], nCities)];

  if (bestCost > currentCost) {
    return currentCost;
  } else {
    return bestCost;
  }
}

/**
 * Update the pheromons in the pheromons matrix given the current path
 *
 * The maximum pheromon value for an edge is 1.
 **/
void updatePheromons(double* pheromons, int* path, long cost, int nCities) {
  int i;
  int* orderedCities = (int*) malloc(nCities*sizeof(int));

  for (i = 0; i < nCities; i++) {
    int order = path[i];
    orderedCities[order] = i;
  }

  for (i = 0; i < nCities - 1; i++) {
    pheromons[getMatrixIndex(orderedCities[i],orderedCities[i + 1], nCities)] += 1.0/cost;
    pheromons[getMatrixIndex(orderedCities[i + 1],orderedCities[i], nCities)] += 1.0/cost;
    if (pheromons[getMatrixIndex(orderedCities[i],orderedCities[i + 1], nCities)] > 1) {
      pheromons[getMatrixIndex(orderedCities[i],orderedCities[i + 1], nCities)] = 1.0;
      pheromons[getMatrixIndex(orderedCities[i + 1],orderedCities[i], nCities)] = 1.0;
    }
  }
  // add last
  pheromons[getMatrixIndex(orderedCities[nCities - 1],orderedCities[0],nCities)] += 1.0/cost;
  pheromons[getMatrixIndex(orderedCities[0],orderedCities[nCities - 1], nCities)] += 1.0/cost;
  if (pheromons[getMatrixIndex(orderedCities[nCities - 1],orderedCities[0],nCities)] > 1.0) {
    pheromons[getMatrixIndex(orderedCities[nCities - 1],orderedCities[0],nCities)] = 1.0;
    pheromons[getMatrixIndex(orderedCities[0],orderedCities[nCities - 1], nCities)] = 1.0;
  }
}

#if __linux__ 

#include <sys/time.h>

double second() {
  struct timeval tp;
  struct timezone tzp;
  gettimeofday(&tp,&tzp);
  return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}
#elif _WIN32

#define M_PI 3.14159265359

double second() {
  SYSTEMTIME st;
  GetSystemTime(&st);
  return ( (double)  st.wSecond + (double) st.wMilliseconds * 1.e-6 );
}
#endif

