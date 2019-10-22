#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "myLib.h"


int main(int argc, char* argv[] ) {
  int sem1, sem2;

  int shmem;
  struct hashNode *sh;
  pid_t pid;
  int status;
  int k, numOfFrames, q, max;
  int bzipEntries = 0, gccEntries = 0, bzipW = 0, gccW = 0, bzipR = 0, gccR = 0, pfGcc = 0, pfBzip = 0, totalPf = 0, diskWrites = 0;
	FILE *fileA, *fileB;
  char str[12], localAddress[6];


  //Get the arguments
	if(argc == 4) {
		k = atoi(argv[1]);
		numOfFrames = atoi(argv[2]);
		q = atoi(argv[3]);
		max = 1000001;
	}
	else if(argc == 5) {
		k = atoi(argv[1]);
		numOfFrames = atoi(argv[2]);
		q = atoi(argv[3]);
		max = atoi(argv[4]);
	}
	else {
		printf("Wrong arguments.\n");
		return -1;
	}

  //Opening files
	if((fileA = fopen("bzip.trace", "r")) == 0) {
		printf("Error in opening bzip.trace.\n");
		return -1;
	}

	if((fileB = fopen("gcc.trace", "r")) == 0) {
		printf("Error in opening gcc.trace.\n");
		return -1;
	}

  //Initialize Semaphores
  sem1 = getSemaphore();
  sem_Init(sem1,1);
  sem2 = getSemaphore();
  sem_Init(sem2,0);

  //Create a new shared memory
  shmem = getSharedMem();
  //Attach the shared memory segments
  sh = attachSharedMem(shmem);


  for(int kid = 0; kid < 3; kid++) {
    pid = fork();
    if(pid < 0){
        exit(EXIT_FAILURE);
    }
    else if (pid == 0){
      /* Child process */
      if(kid == 0){
        //Read from bzip.trace
        int i=0;
        while(i < q && bzipEntries < max) {
          fgets(str, sizeof(str), fileA);

          if(feof(fileA) !=0 ) {
            fclose(fileA);
            break;
          }


    			strncpy(localAddress, str, 5);
    			localAddress[5] = '\0';

          sem_down(sem1);

          sh->kid = kid + 1;
          strcpy(sh->localAddress, localAddress);
          sh->type = str[9];
          
          sem_up(sem2);

          i++;
        }

        exit(EXIT_SUCCESS);
      }
      else if(kid == 1){
        //Read from gcc.trace
        int i=0;
        while(i < q && gccEntries < max) {
          fgets(str, sizeof(str), fileB);

          if(feof(fileB) !=0 ) {
            fclose(fileB);
            break;
          }


    			strncpy(localAddress, str, 5);
    			localAddress[5] = '\0';


          sem_down(sem1);

          sh->kid = kid + 1;
          strcpy(sh->localAddress, localAddress);
          sh->type = str[9];

          sem_up(sem2);

          i++;
        }

        exit(EXIT_SUCCESS);
      }
      else if(kid == 2){

        int i=0, totalEntries=0;
        struct hashNode* hashNode = malloc(sizeof(struct hashNode));
        struct hashNode* hashTable = malloc(numOfFrames*sizeof(struct hashNode));
        for(int j=0; j<numOfFrames; j++){
          hashTable[j].next = NULL;
        }
        while(i < 2*q && totalEntries < 2*max) {

          sem_down(sem2);

          hashNode->kid = sh->kid;
          strcpy(hashNode->localAddress, sh->localAddress);
          hashNode->type = sh->type;

          if(sh->kid == 2){
            if(sh->type == 'W'){
              gccW++;
            }
            else {
              gccR++;
            }
          }else{
            if(sh->type == 'W'){
              bzipW++;
            }
            else {
              bzipR++;
            }
          }


          sem_up(sem1);

          if(!searchInHashTable(hashTable, hashNode, numOfFrames)){
            if(hashNode->kid == 1){
              pfBzip++;
              bzipEntries++;
            }
            else{
              pfGcc++;
              gccEntries++;
            }
            totalPf++;

            insertInHashTable(hashTable, hashNode, numOfFrames, &totalPf, &diskWrites, k);
          }

          i++;
          totalEntries++;
        }
        printf("Total disk writes: %d\n", diskWrites);
      	printf("Total requests to write to disk: %d\n", gccW + bzipW);
      	printf("Total requests to read from disk: %d\n", gccR + bzipR);
        printf("Total Page Faults: %d\n", pfBzip + pfGcc);
      	printf("From gcc.trace read %d entries.\n", gccEntries);
      	printf("From bzip.trace read %d entries.\n", bzipEntries);
      	printf("Total number of occupied frames: %d\n", TotalNumOfFramesInHash(hashTable,numOfFrames));
        deleteHashTable(hashTable, numOfFrames);
        free(hashNode);
        exit(EXIT_SUCCESS);
      }
    }
    else {
      /* parent process */
      continue;
    }
  }

  while(wait(&status)>0);

	fclose(fileA);
	fclose(fileB);

  // Detach and delete shared memory
  detachSharedMem(sh);
  deleteSharedMem(shmem);

  // Delete semaphores
  deleteSemaphore(sem1);
  deleteSemaphore(sem2);
}
