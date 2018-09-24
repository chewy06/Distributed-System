#include <stdio.h>
#include <ctype.h>
#include "mpi.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/*This program will calculate the shortes path to the Traveling Salesman Problem(TSP). 
 * Using openmpi, the program will create a number of nodes that will generate
 * unique random tours. Based on the random tour new tours will be generated. The tour
 * with the lowest cost will be broadcasted in order to find the tour with the lowest cost.*/

/* struct to store details about the city nodes.*/
typedef struct {
  float x;
  float y;
  int id;
  int visited;  
} CITY;

void tsp_start(int rank, int proc_size);
void parse(FILE *file);
void genRandomPath();
void genNewPaths();
void copyForward(int i,int j);
void copyBackwards(int i,int j);
void displayResults(int root,int rank);
void myBcast(int root,int proc_size);
float getCost(CITY a,CITY b);
float calculate_total_cost(CITY *cities);

CITY *cities;
CITY *randPath;
CITY *currentPath;
CITY *bestPath;

int size;
float bestCost;

int main(int argc, char **argv){
  FILE *fp;
  int i,proc_size, my_rank,iteration;
  char *file;

  if(argc<3){
    printf("usage: myprog input_file arg1\n");
    return 0;
  }

  file=*++argv;
  iteration=atoi(*++argv);
  fp = fopen(file,"r");

  if(fp == NULL) {
    printf("File <%s> not found\n",file);
    return 0;
  }
    
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD,&proc_size);
  MPI_Comm_rank(MPI_COMM_WORLD,&my_rank);
  
  bestCost=0;
  currentPath=NULL;
  randPath=NULL;

  /* Parse file into city location in terms of x and y*/
  parse(fp);
  if (my_rank==0)
    printf("Number of Cities: %d\n",size);

  /* Number iterations to do.*/
  for(i=0;i<iteration;i++)
    tsp_start(my_rank,proc_size);

  myBcast(my_rank,proc_size);
  fclose(fp);
  MPI_Finalize();

  return 0; 
}

/*Function to generate random tour and generate new tours.*/
void tsp_start(int rank, int proc_size){
  srand(time(NULL)+rank);
  genRandomPath();
  genNewPaths();
}

/*Broadcast result for all nodes to rank==0 and get shortest tour.*/
void myBcast(int root,int proc_size){
 int i,rank=0;
 float temp=0,best=bestCost;

 if(root!=0) {
   printf("rank %d sending data...\n",root);
   MPI_Send(&bestCost,1,MPI_FLOAT,0,0,MPI_COMM_WORLD);
 }else {
   printf("rank %d receiving data...\n",root);
   for(i=1;i<proc_size;i++){
     MPI_Recv(&temp,1,MPI_FLOAT,i,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
     if(temp<best){
       best=temp;
       rank=i;
     }
   }
 }
 /* Rebroadcast the rank id node with the lowest cost in order to print results.*/
 MPI_Bcast(&rank,1,MPI_INT,0,MPI_COMM_WORLD);
 displayResults(root,rank);
}

/* Display the tour from the rank with the lowest tour*/
void displayResults(int root, int rank){
  int index;

  if(root==rank) {
    printf("Best Tour Rank: %d Cost: %.1f\n",root,bestCost);
    for(index=0;index<size;index++)
      printf("%d\n",bestPath[index].id);
  }
}

/* Extracting data from file*/
void parse(FILE *file) {
  CITY *tempCity;
  char line[1024];
  int index,i;
  float x,y;

  /* Skipping the first 6 lines of the file.*/
  for (i=0; i<6;i++){
    fgets(line,1024,file);
  }

  /* Getting the number of cities.*/
  while(fscanf(file,"%d %f %f",&index,&x,&y)>0);

  cities=(CITY*)malloc(sizeof(CITY)*index);
  bestPath=(CITY*)malloc(sizeof(CITY)*index);
  currentPath=malloc(sizeof(CITY)*index);
  size=index;
  tempCity=cities;

  /* Set file ptr at beggining*/
  rewind(file);

  /* Skip first 6 lines AGAIN.*/
  for (i=0; i<6;i++){
    fgets(line,1024,file);
  }

  /* Scan data and store in cities ptr.*/
  while(fscanf(file,"%d %f %f",&index,&x,&y)>0){
    tempCity->id=index;
    tempCity->x=x;
    tempCity->y=y;
    tempCity->visited=0;
    tempCity++;
  }
}

/* Generating random path.*/
void genRandomPath(){
  CITY *tempPath;
  int tempSize=size;
 
  if(randPath==NULL) 
    randPath=calloc(sizeof(CITY),size);
  
  tempPath=randPath;

  /* Loop untill all cities have been randomly assign.*/
  while(tempSize) {
    int r = rand() % size;

    /* If the city has not been visited add to the random tour.*/
    if (!cities[r].visited){
      memcpy(tempPath,&cities[r],sizeof(CITY));
      cities[r].visited=1;
      tempSize--;   
      tempPath++;
    }
  }

  /* Calculate shortest path*/
  calculate_total_cost(randPath);

  /* Reset cities visited to NOT VISITED.*/
  for(tempSize=0;tempSize<size;tempSize++)
    cities[tempSize].visited=0;
}

/* Calculates the cost between two cities.*/
float getCost(CITY a, CITY b){
  float dx=a.x-b.x;
  float dy=a.y-b.y;

  float s=dx*dx+dy*dy;

  return sqrt(s);
}

/* Extracts location from structs and stores best cost.*/
float calculate_total_cost(CITY *locations){
  int index,total_cost=0;

  /* Extracting locations and calculating distance between tour path.*/
  for (index=1;index<size;index++){
    CITY a=locations[index-1];
    CITY b=locations[index];
    total_cost+=getCost(a,b);      
  }
  total_cost+=getCost(locations[index-1],locations[0]);

  /* Stores best path and best cost.*/
  if(bestCost==0) {
    bestCost=total_cost;
    memcpy(bestPath,locations,sizeof(CITY)*size);
  } else if (total_cost<bestCost) {
    bestCost=total_cost;
    memcpy(bestPath,locations,sizeof(CITY)*size);
  }
}
/* Follows heuristic: creates new path from Cj->Ci+1*/
void copyBackwards(int i, int j){
  int max=j,x;
  for(x=i;x<=max;x++){
    memcpy(&currentPath[x],&randPath[j--],sizeof(CITY));
  }
}

/* Follows heuristic: continues Cj->C(size-j)*/
void copyForward(int i, int j){
  memcpy(&currentPath[j+1],&randPath[j+1],sizeof(CITY)*(size-j));
}

/* Generates new paths using spec heuristics.*/
void genNewPaths(){
  int i,j;

  for(i=1;i<size;i++){
    for(j=i;j<size;j++){
      memcpy(currentPath,randPath,sizeof(CITY)*size);
      copyBackwards(i,j);
      copyForward(i,j);
      calculate_total_cost(currentPath);
    }  
  }
}
