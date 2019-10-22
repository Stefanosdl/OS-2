//Union semun
union semun {
   int val;                  //value for SETVAL
   struct semid_ds *buf;     // buffer for IPC_STAT, IPC_SET
   unsigned short *array;    // array for GETALL, SETALL
};

struct hashNode {
  int kid;                  //number of process
  char localAddress[6];
  char type;
  struct hashNode* next;
};

void print(struct hashNode* hashTable, int numOfFrames);
int TotalNumOfFramesInHash(struct hashNode* , int );
int searchInHashTable(struct hashNode* , struct hashNode* , int );
void insertInHashTable(struct hashNode* , struct hashNode* , int , int* , int* , int );
void deleteHashTable(struct hashNode* , int );
int sem_down(int );
int sem_up(int );
void sem_Init(int , int );
int getSemaphore();
int getSharedMem();
struct hashNode* attachSharedMem(int );
void deleteSemaphore(int );
void detachSharedMem(struct hashNode* );
void deleteSharedMem(int );
