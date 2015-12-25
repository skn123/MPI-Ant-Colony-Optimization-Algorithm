/* Ant Colony Optimixzation Algorithm - Marc Schaer*/
#include <mpi.h>
#include "utils.h"

int main(int argc, char* argv[]) {

  if (argc != 9) {
    printf("use : %s mapFile randomNumberFile nbAnts nbExternalIterations nbOnNodeIterations alpha beta evaporationCoeff\n", argv[0]);
    return -1;
  }

  int prank, psize;

  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &prank);
  MPI_Comm_size(MPI_COMM_WORLD, &psize);

  if (prank == 0) {
    printf("NbOfNodes %d\n", psize);
  }

  int i, j, ant_counter, cities_counter;
  long loop_counter;
  long random_counter = 0;
  long external_loop_counter = 0;
  int *map = NULL;
  double *pheromons;
  int* pheromonsUpdate;
  // bestPath is a vector representing all cities in order.
  // If the value is 0, the city was not visited
  // else, the city is visited at step i
  int *bestPath;
  int *otherBestPath;
  int *currentPath;
  long bestCost = INFTY;
  long otherBestCost;
  double* localPheromonsPath;
  double* otherPheromonsPath;

  long* randomNumbers;
  long nRandomNumbers = 0;

  char* mapFile;
  char* randomFile;
  int nAnts; 
  long externalIterations; 
  long onNodeIteration; 
  double alpha; 
  double beta; 
  double evaporationCoeff; 
  int nCities = 0;
  int* nAntsPerNode;
  long terminationCondition = 0;
  long otherTerminationCondition = 0;
  double terminationConditionPercentage = 0.4;

  // share random file first
  if (prank == 0) {
    std::ifstream in;
    randomFile = argv[2];
    printf("RandomFile %s\n", randomFile);
    char out[20];

    // Read random numbers file
    in.open(randomFile);

    if (!in.is_open()) {
      printf("Cannot open file.\n");
      printf("The filepath %s is incorrect\n", randomFile);
      MPI_Finalize();
      return -1;
    }

    in >> out;

    // Define number of random numbers
    nRandomNumbers = atol(out);

    // Allocation of random numbers vector
    randomNumbers = (long*) malloc(nRandomNumbers*sizeof(long));

    for (i = 0; i < nRandomNumbers; i++) {
      randomNumbers[i] = 0;
    }

    i = 0;
    while(!in.eof()) {
      in >> out;
      randomNumbers[i] = atol(out);
      i++;
    }

    in.close();

  }

  if (MPI_Bcast(&nRandomNumbers, 1, MPI_LONG, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of nRandomNumbers", prank);
    MPI_Finalize();
    return -1;
  }

  if (prank != 0) {
    randomNumbers = (long*) malloc(nRandomNumbers*sizeof(long));
    for (i = 0; i < nRandomNumbers; i++) {
      randomNumbers[i] = 0;
    }
  }

  if (MPI_Bcast(&randomNumbers[0], nRandomNumbers, MPI_LONG, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of randomNumbers", prank);
    MPI_Finalize();
    return -1;
  }

  // Start Timing
  if (prank == 0) {
    start = second();
  }

  nAntsPerNode = (int*) malloc(psize*sizeof(int));

  // Root reads args
  if (prank == 0) {
    mapFile = argv[1];
    nAnts = atoi(argv[3]);
    externalIterations = atoi(argv[4]);
    onNodeIteration = atoi(argv[5]);
    alpha = atof(argv[6]);
    beta = atof(argv[7]);
    evaporationCoeff = atof(argv[8]);

    int antsPerNode = nAnts / psize;
    int restAnts = nAnts - antsPerNode * psize;

    for (i = 0; i < psize; i++) {
      nAntsPerNode[i] = antsPerNode;
      if (restAnts > i) {
        nAntsPerNode[i]++;
      }
    }

    printf("Iterations %ld\n", externalIterations*onNodeIteration);
    printf("Ants %d\n", nAnts);

    // Load the map and the number of cities
    std::ifstream in;
    in.open(mapFile);

    if (!in.is_open()) {
      printf("Cannot open file.\n");
      printf("The filepath %s is incorrect\n", mapFile);
      MPI_Finalize();
      return -1;
    }
    char out[12];
    in >> out;

    // Define number of cities
    nCities = atoi(out);

    printf("Cities %d\n", nCities);

    // Allocation of local map
    map = (int*) malloc(nCities*nCities*sizeof(int));

    in.close();

    // Load the map inside map variable
    if (LoadCities(mapFile, map)) {
      printf("The filepath is incorrect\n");
      MPI_Finalize();
      return -1;
    }

    // printf("Number of cities : %d\n", nCities);
  }

  // And then it shares values with other nodes
  if (MPI_Bcast(&nAntsPerNode[0], psize, MPI_INT, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of nAntsPerNode", prank);
    MPI_Finalize();
    return -1;
  }
  if (MPI_Bcast(&onNodeIteration, 1, MPI_LONG, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of onNodeIteration", prank);
    MPI_Finalize();
    return -1;
  }
  if (MPI_Bcast(&externalIterations, 1, MPI_LONG, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of externalIterations", prank);
    MPI_Finalize();
    return -1;
  }
  if (MPI_Bcast(&alpha, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of alpha", prank);
    MPI_Finalize();
    return -1;
  }
  if (MPI_Bcast(&beta, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of beta", prank);
    MPI_Finalize();
    return -1;
  }
  if (MPI_Bcast(&evaporationCoeff, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of evaporationCoeff", prank);
    MPI_Finalize();
    return -1;
  }


  // Share number of cities
  if (MPI_Bcast(&nCities, 1, MPI_INT, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of nCities", prank);
    MPI_Finalize();
    return -1;
  }

  // Allocation of map for non-root nodes
  if (prank != 0) {
    map = (int*) malloc(nCities*nCities*sizeof(int));
  }


  if (MPI_Bcast(&map[0], nCities * nCities, MPI_INT, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
    printf("Node %d : Error in Broadcast of map", prank);
    MPI_Finalize();
    return -1;
  }

  // Allocation of pheromons
  pheromons = (double*) malloc(nCities*nCities*sizeof(double));
  pheromonsUpdate = (int*) malloc(nCities*nCities*sizeof(int));
  localPheromonsPath = (double*) malloc(nCities*sizeof(double));

  otherBestPath = (int*) malloc(nCities*sizeof(int));
  otherPheromonsPath = (double*) malloc(nCities*sizeof(double));
  for (i = 0; i < nCities; i++) {
    otherBestPath[i] = -1;
  }

  bestPath = (int*) malloc(nCities*sizeof(int));
  currentPath = (int*) malloc(nCities*sizeof(int));

  // Initialisation of pheromons and other vectors
  for (i = 0; i < nCities; i++) {
    currentPath[i] = -1;
    bestPath[i] = -1;
  }
  for (i = 0; i < nCities * nCities; i++) {
    pheromons[i] = 0.1;
  }

  nAnts = nAntsPerNode[prank];
  int totalNAnts = 0;
  for (i = 0; i < psize; i++) {
    totalNAnts += nAntsPerNode[i];
  }

  int nAntsBeforeMe = 0;
  for (i = 0; i < psize; i++) {
    if (i < prank) {
      nAntsBeforeMe += nAntsPerNode[i];
    } else {
      i = psize;
    }
  }

  random_counter = (random_counter + (onNodeIteration * nAntsBeforeMe * nCities)) % nRandomNumbers;

  long antsBestCost = INFTY;

  printf("%ld\n",(long) ceilf(totalNAnts * externalIterations * onNodeIteration * terminationConditionPercentage)); 

  while (external_loop_counter < externalIterations && terminationCondition < (long) ceilf(totalNAnts * externalIterations * onNodeIteration * terminationConditionPercentage)) {

    loop_counter = 0;
    while (loop_counter < onNodeIteration) {

      printf("Loop nr. : %ld in node %d, terminationCondition : %ld, bestCost : %ld\n", external_loop_counter, prank, terminationCondition,bestCost);

      // Loop over each ant
      for (ant_counter = 0; ant_counter < nAnts; ant_counter++) {
        // init currentPath 
        // -1 means not visited
        for (i = 0; i < nCities; i++) {
          currentPath[i] = -1;
        }

        // select a random start city for an ant
        long rand = randomNumbers[random_counter];
        int currentCity = rand % nCities;
        printf("%d\n", currentCity);
        random_counter = (random_counter + 1) % nRandomNumbers;
        // currentPath will contain the order of visited cities
        currentPath[currentCity] = 0;
        for (cities_counter = 1; cities_counter < nCities; cities_counter++) {
          // Find next city
          rand = randomNumbers[random_counter];
          random_counter = (random_counter + 1) % nRandomNumbers;
          currentCity = computeNextCity(currentCity, currentPath, map, nCities, pheromons, alpha, beta, rand);
          printf("%d\n", currentCity);

          if (currentCity == -1) {
            printf("There is an error choosing the next city in iteration %ld for ant %d on node %d\n", loop_counter, ant_counter, prank);
            MPI_Finalize();
            return -1;
          }

          // add next city to plan
          currentPath[currentCity] = cities_counter;
        }

        // update bestCost and bestPath
        long oldCost = bestCost;
        bestCost = updateBestPath(bestCost, bestPath, currentPath, map, nCities);

        if (oldCost > bestCost) {
          copyVectorInt(currentPath, bestPath, nCities);
        }
      }

      if (bestCost < antsBestCost) {
        antsBestCost = bestCost;
        terminationCondition = 0;
      } else {
        terminationCondition++;
      }

      // Pheromon evaporation
      for (i = 0; i < nCities * nCities; i++) {
        pheromons[i] *= evaporationCoeff;
      }
      // Update pheromons
      updatePheromons(pheromons, bestPath, bestCost, nCities);

      loop_counter++;
    }

    findPheromonsPath(localPheromonsPath, bestPath, pheromons, nCities);

    for (j = 0; j < nCities*nCities; j++) {
      pheromonsUpdate[j] = 1;
    }
    long tempBestCost = bestCost;
    int* tempBestPath = (int*) malloc(nCities * sizeof(int));
    long tempTerminationCondition = terminationCondition;
    copyVectorInt(bestPath, tempBestPath, nCities);
    for (i = 0; i < psize; i++) {
      if (prank == i) {
        copyVectorInt(bestPath, otherBestPath, nCities);
        copyVectordouble(localPheromonsPath, otherPheromonsPath, nCities);
        otherTerminationCondition = terminationCondition;
        otherBestCost = bestCost;
      }
      if (MPI_Bcast(&otherBestPath[0], nCities, MPI_INT, i, MPI_COMM_WORLD) != MPI_SUCCESS) {
        printf("Node %d : Error in Broadcast of otherBestPath", prank);
        MPI_Finalize();
        return -1;
      }
      if (MPI_Bcast(&otherPheromonsPath[0], nCities, MPI_DOUBLE, i, MPI_COMM_WORLD) != MPI_SUCCESS) {
        printf("Node %d : Error in Broadcast of otherPheromonsPath", prank);
        MPI_Finalize();
        return -1;
      }
      if (MPI_Bcast(&otherTerminationCondition, 1, MPI_LONG, i, MPI_COMM_WORLD) != MPI_SUCCESS) {
        printf("Node %d : Error in Broadcast of otherTerminationCondition", prank);
        MPI_Finalize();
        return -1;
      }
      if (MPI_Bcast(&otherBestCost, 1, MPI_LONG, i, MPI_COMM_WORLD) != MPI_SUCCESS) {
        printf("Node %d : Error in Broadcast of otherBestCost", prank);
        MPI_Finalize();
        return -1;
      }

      if (prank != i) {
        if (otherBestCost < tempBestCost) {
          tempTerminationCondition = otherTerminationCondition;
          tempBestCost = otherBestCost;
          copyVectorInt(otherBestPath, tempBestPath,  nCities);
        } else if (otherBestCost == tempBestCost) {
          tempTerminationCondition += otherTerminationCondition;
        }

        for (j = 0; j < nCities - 1; j++) {
          pheromonsUpdate[getMatrixIndex(otherBestPath[j],otherBestPath[j+1],nCities)] += 1;
          pheromonsUpdate[getMatrixIndex(otherBestPath[j+1],otherBestPath[j],nCities)] += 1;
          pheromons[getMatrixIndex(otherBestPath[j],otherBestPath[j+1],nCities)] += otherPheromonsPath[j];
          pheromons[getMatrixIndex(otherBestPath[j+1],otherBestPath[j],nCities)] += otherPheromonsPath[j];
        }
        pheromonsUpdate[getMatrixIndex(otherBestPath[nCities-1],otherBestPath[0],nCities)] += 1;
        pheromonsUpdate[getMatrixIndex(otherBestPath[0],otherBestPath[nCities-1],nCities)] += 1;
        pheromons[getMatrixIndex(otherBestPath[nCities-1],otherBestPath[0],nCities)] += otherPheromonsPath[nCities - 1];
        pheromons[getMatrixIndex(otherBestPath[0],otherBestPath[nCities-1],nCities)] += otherPheromonsPath[nCities - 1];
      }
    }

    for (j = 0; j < nCities*nCities; j++) {
      pheromons[j] = pheromons[j] / pheromonsUpdate[j];
    }

    bestCost = tempBestCost;
    copyVectorInt(tempBestPath, bestPath, nCities);
    terminationCondition = tempTerminationCondition;


    external_loop_counter++;

    random_counter = (random_counter + (onNodeIteration * (totalNAnts - nAnts) * nCities)) % nRandomNumbers;
  }

  // Merge solution into root 
  if (prank == 0) {
    for (i = 1; i < psize; i++) {
      if (MPI_Recv(&otherBestPath[0], nCities, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status) != MPI_SUCCESS) {
        printf("Node %d : Error in Recv of otherBestPath", prank);
        MPI_Finalize();
        return -1;
      }
      long oldCost = bestCost;
      bestCost = updateBestPath(bestCost, bestPath, otherBestPath, map, nCities);

      if (oldCost > bestCost) {
        copyVectorInt(otherBestPath, bestPath, nCities);
      }
    }
  } else {
    if (MPI_Send(&bestPath[0], nCities, MPI_INT, 0, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
      printf("Node %d : Error in Send of bestPath", prank);
      MPI_Finalize();
      return -1;
    }
  }


  if (prank == 0) {
    // printPath(bestPath, nCities);
    printf("best cost : %ld\n", bestCost);
  }

  if (prank == 0) {
    end = second();
    printf("TotalTime %f\n", (end - start));
  }

  // deallocate the pointers
  free(randomNumbers);
  free(map);
  free(pheromons);
  free(localPheromonsPath);
  free(otherPheromonsPath);
  free(bestPath);
  free(otherBestPath);

  MPI_Finalize();

  return 0;
}

