#include <stdio.h>
#include <stdbool.h>

#define BLOCK_SIZE 5;
#define MAX_BLOCKS 10;

typedef struct {
    int nbRecords;
    Recorde data[BLOCK_SIZE];
    bool is_free; // true is free else not free
    int index;
} Block, Buffer;

typedef struct {
    int BlockIndex;
    bool status;
} TABAllocation;

typedef struct {
    int id;
    char data[20];
    bool is_deleted; // true is free else not free
} Recorde;

typedef struct {
    Block blocks[MAX_BLOCKS]; // array of the blocks that we have in the MS
    int nbBlocks; // number of blocks
    int nbOcc; // number of occupied blocks
} MS;
 
void initMS(MS ms,TABAllocation tb,int nbBblocks) {
    ms.nbBlocks = nbBblocks;
    ms.nbOcc = 0;
    tb.BlockIndex = 0;
    tb.status = false;
}

// void createBlock(int nbRecords, Recorde data[], bool is_free, int index) {

// }

void check(FILE file) {
    MS ms;
    fread(&ms,sizeof(ms),1,file);
    fseek(file,sizeof(ms) + ms.nbBlocks sizeof(Block),SEEK_SET);
    Buffer buffer;
    fread(&buffer,sizeof(buffer),1,file);
    if (buffer.nbRecords < BLOCK_SIZE) {
        fseek(file,sizeof(Recorde) * buffer.nbRecords,SEEK_CUR);
    } else {
        // checking if scondary memory is full then no place else acesse the new Block 
    }
}