#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include "myLib.h"

int searchInHashTable(struct hashNode* hashTable, struct hashNode* hashNode, int numOfFrames) {
  int key;

  sscanf(hashNode->localAddress, "%d", &key);
  key = key % numOfFrames;
  struct hashNode* head = hashTable[key].next;

    while(head != NULL){
      if(head->kid == hashNode->kid && (strcmp(head->localAddress,hashNode->localAddress) == 0)){
        if(head->type == 'R' && hashNode->type == 'W'){
          head->type = hashNode->type;
        }
        return 1;
      }
      head = head->next;
    }
    return 0;
}

void print(struct hashNode* hashTable, int numOfFrames){
  struct hashNode* head, *current;
  for(int i=0;i<numOfFrames;i++){
    head = hashTable[i].next;
    while(head != NULL){
      printf("NODE dd:%s kid:%d ",head->localAddress, head->kid);
      head=head->next;
    }
    printf("%s\n","NULL");
  }
}


int TotalNumOfFramesInHash(struct hashNode* hashTable, int numOfFrames){
  struct hashNode* head;
  int counter = 0;
  for(int i=0; i<numOfFrames; i++){
    head = hashTable[i].next;
    while(head != NULL){
      counter++;
      head = head->next;
    }
  }
  return counter;
}

void insertInHashTable(struct hashNode* hashTable, struct hashNode* hashNode, int numOfFrames, int* totalPf , int* diskWrites , int k) {
  int key;
  struct hashNode* temp;
  sscanf(hashNode->localAddress, "%d", &key);
  key = key % numOfFrames;
  //if there are no nodes at the current position
  if(hashTable[key].next == NULL){
    //insert data
    temp = malloc(sizeof(struct hashNode));
    temp->kid = hashNode->kid;
    strcpy(temp->localAddress, hashNode->localAddress);
    temp->type = hashNode->type;
    temp->next = NULL;
    //point to the first node
    hashTable[key].next = temp;
  }
  else{
    struct hashNode* head = hashTable[key].next;
    //find the last node
    while(head->next != NULL){
      head = head->next;
    }
    //insert data
    temp = malloc(sizeof(struct hashNode));
    temp->kid = hashNode->kid;
    strcpy(temp->localAddress, hashNode->localAddress);
    temp->type = hashNode->type;
    temp->next = NULL;
    //the last node we found will point to the new one
    head->next = temp;
  }



  // //FWF
  if((*totalPf) == k + 1){
    struct hashNode* temp;
    struct hashNode* prev;
    for(int i=0; i<numOfFrames; i++){
      temp = hashTable[i].next;

      // If head node itself holds the key or multiple occurrences of key
      while(temp != NULL && temp->kid == hashNode->kid){
        if(temp->type == 'W'){
           (*diskWrites)++;
         }
        // Changed head
        hashTable[i].next = temp->next;
        free(temp);
        // Change Temp
        temp = hashTable[i].next;
        continue;
      }
      // Delete occurrences other than head
      while(temp != NULL){

        // Search for the key to be deleted, keep track of the
        // previous node as we need to change 'prev->next'
        while(temp != NULL && temp->kid != hashNode->kid){
          prev = temp;
          temp = temp->next;

        }

        // If key was not present in linked list
        if(temp == NULL){
          continue;
        }

        // Unlink the node from linked list
        prev->next = temp->next;

        if(temp->type == 'W'){
           (*diskWrites)++;
        }
        free(temp);

        //Update Temp for next iteration of outer loop
        temp = prev->next;

      }

    }
    (*totalPf) = 0;
  }
}

void deleteHashTable(struct hashNode* hashTable, int numOfFrames){
  struct hashNode* current;
  struct hashNode* next;
  for(int i=0; i<numOfFrames; i++){
    current = hashTable[i].next;
    while(current != NULL){
      next = current->next;
      free(current);
      current = next;
    }
  }
  free(hashTable);
}

//Semaphore down operation, using semop
int sem_down(int sem_id) {
   struct sembuf sem_d;
   sem_d.sem_num = 0;
   sem_d.sem_op = -1;
   sem_d.sem_flg = 0;
   if (semop(sem_id,&sem_d,1) == -1) {
       perror("Semaphore Down Operation ");
       return -1;
   }
   return 0;
}

//Semaphore up operation, using semop
int sem_up(int sem_id) {
   struct sembuf sem_d;
   sem_d.sem_num = 0;
   sem_d.sem_op = 1;
   sem_d.sem_flg = 0;
   if (semop(sem_id,&sem_d,1) == -1) {
       perror("Semaphore Up Operation ");
       return -1;
   }
   return 0;
}

//Semaphore Init - set a semaphore's value to val
void sem_Init(int sem_id, int val) {
   union semun arg;
   arg.val = val;
   if (semctl(sem_id,0,SETVAL,arg) == -1) {
       perror("Semaphore Setting Value ");
       exit(0);
   }
}

int getSemaphore() {
    int semId;
    if((semId = semget((key_t)IPC_PRIVATE,1,0600|IPC_CREAT)) < 0){
      perror("Semaphore Creation");
			semctl(semId,0,IPC_RMID);
			exit(0);
    }
    return semId;
}

int getSharedMem() {
  int shmem;

  if((shmem=shmget(IPC_PRIVATE, sizeof(struct hashNode),0666| IPC_CREAT)) < 0)
  {
    perror("Error in input shared memory creation");
    shmctl(shmem,IPC_RMID,0);
    exit(0);
  }
  return shmem;
}


struct hashNode* attachSharedMem(int shmem) {
   struct hashNode *sh;
   sh = (struct hashNode*)shmat(shmem,(struct hashNode*)0,0);
   if (sh == (struct hashNode*)-1 ) {
     perror("Error in shared memory attach");
     shmctl(shmem,IPC_RMID,0);
     exit(0);
   }
   return sh;
}

void deleteSemaphore(int sem) {
  if (semctl(sem,0,IPC_RMID) == -1) {
    perror("Error in semaphore deleting");
      exit(0);
  }
}

void detachSharedMem(struct hashNode* sh) {
    shmdt((char*)sh);
}

void deleteSharedMem(int shmem) {
  if(shmctl(shmem,IPC_RMID,0) == -1){
      perror("Failed To Delete INPUT Shared Memory");
    exit(0);
    }
}
