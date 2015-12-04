#include "utils.h"

int main(int argc, char* argv[]) {

  if (argc != 8) {
    printf("use : %s mapFile randomNumberFile nbAnts nbIterations alpha beta evaporationCoeff\n", argv[0]);
    return -1;
  }


  int i, j, loop_counter, ant_counter, cities_counter;
  int random_counter = 0;
  int **map = NULL;
  float **pheromons;
  // bestPath is a vector representing all cities in order.
  // If the value is 0, the city was not visited
  // else, the city is visited at step i
  int *bestPath;
  int *currentPath;
  int bestCost = INFTY;
  int* randomNumbers;

  char* mapFile = argv[1];
  char* randomFile = argv[2];
  int nAnts = atoi(argv[3]);;
  int iterations = atoi(argv[4]);
  float alpha = atof(argv[5]);
  float beta = atof(argv[6]);
  float evaporationCoeff = atof(argv[7]);
  int nCities = 0;
  int nRandomNumbers = 0;

  start = second();

  printf("Iterations %d\n", iterations);
  printf("Ants %d\n", nAnts);

  // Load the map and the number of cities
  std::ifstream in;
  in.open(mapFile);

  if (!in.is_open()) {
    printf("Cannot open file.\n");
    printf("The filepath %s is incorrect\n", mapFile);
    return -1;
  }
  char out[12];
  in >> out;

  // Define number of cities
  nCities = atoi(out);

  printf("Cities %d\n", nCities);

  // Allocation of map
  map = (int**) malloc(nCities*sizeof(int*));
  for (i = 0; i < nCities; i++) {
    map[i] = (int*) malloc(nCities*sizeof(int));

  }

  in.close();

  // Read random number file
  in.open(randomFile);

  if (!in.is_open()) {
    printf("Cannot open file.\n");
    printf("The filepath %s is incorrect\n", randomFile);
    return -1;
  }

  in >> out;

  // Define number of random numbers
  nRandomNumbers = atoi(out);

  // Allocation of random numbers vectors
  randomNumbers = (int*) malloc(nRandomNumbers*sizeof(int));

  i = 0;
  while (!in.eof()) {
    in >> out;
    randomNumbers[i] = atol(out);
    i++;
  }

  // Load the map inside map variable
  if (LoadCities(mapFile, map)) {
    printf("The filepath %s is incorrect\n", mapFile);
    return -1;
  }


  // Allocation of pheromons
  pheromons = (float**) malloc(nCities*sizeof(float*));

  for (i = 0; i < nCities; i++) {
    pheromons[i] = (float*) malloc(nCities*sizeof(float));
  }
  bestPath = (int*) malloc(nCities*sizeof(int));
  currentPath = (int*) malloc(nCities*sizeof(int));

  // Initialisation of pheromons and other vectors
  for (i = 0; i < nCities; i++) {
    currentPath[i] = -1;
    bestPath[i] = -1;
    for (j = 0; j < nCities; j++) {
      pheromons[i][j] = 0.1;
    }
  }

  loop_counter = 0;

  // External loop
  while (loop_counter < iterations) {

    // printf("Loop nr. : %d\n", loop_counter);

    // Loop over each ant
    for (ant_counter = 0; ant_counter < nAnts; ant_counter++) {
      // init currentPath 
      // -1 means not visited
      for (i = 0; i < nCities; i++) {
        currentPath[i] = -1;
      }

      // select a random start city for an ant
      int currentCity = randomNumbers[random_counter]% nCities;
      random_counter = (random_counter + 1) % nRandomNumbers;
      // currentPath will contain the order of visited cities
      currentPath[currentCity] = 0;
      for (cities_counter = 1; cities_counter < nCities; cities_counter++) {
        // Find next city
        currentCity = computeNextCity(currentCity, currentPath, map, nCities, pheromons, alpha, beta);

        if (currentCity == -1) {
          printf("There is an error choosing the next city in interation %d fot ant %d\n", loop_counter, ant_counter);
          return -1;
        }

        // add next city to plan
        currentPath[currentCity] = cities_counter;
      }

      // update bestCost and bestPath
      int oldCost = bestCost;
      bestCost = updateBestPath(bestCost, bestPath, currentPath, map, nCities);

      if (oldCost > bestCost) {
        copyVector(currentPath, bestPath, nCities);
      }
    }
    //
    // Pheromon evaporation
    for (i = 0; i < nCities; i++) {
      for (j = i + 1; j < nCities; j++) {
        pheromons[i][j] *= evaporationCoeff;
        pheromons[j][i] *= evaporationCoeff;
      }
    }
    // Update pheromons
    updatePheromons(pheromons, bestPath, bestCost, nCities);

    loop_counter++;
  }

  //printPath(bestPath, nCities);
  //printf("best cost : %d\n", bestCost);

  end = second();
  printf("Total time : %f\n", (end-start));

  // deallocation of the rows
  for(i=0; i<nCities ;i++) {
    free(map[i]);
    free(pheromons[i]);
  }

  // deallocate the pointers
  free(map);
  free(pheromons);
  free(bestPath);
  free(currentPath);

  return 0;
}


