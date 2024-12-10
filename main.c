#include<stdio.h>
#include<string.h>
#include<stdbool.h>


// ************************************************************
// ************************************************************

#define MaxBlocks 30;

typedef struct {
    int id;
    char data[30];
    bool isDeleted;
} enregistrement;

typedef struct {
    enregistrement tab[3];
    int address; 
    int nbEnregistrement;
    bool isFree;
}block, buffer;

typedef struct {
    char fileName[40];
    int fileId;
    int sizeBlocks;
    int sizeEnrgs;
    int addressFirst;
    bool modeOrganizationGlobal; //chaine / contegu;
    bool modeOrganizationIntern; // trier / !trier;
}metaData;

typedef struct {
    int fileId;
} file;

typedef struct {
    int address;
    bool free // true for free and false for !true;
}BlockState;

BlockState Ms[MaxBlocks]

// ***********************************************************************
// ***********************************************************************

void initialisationDisque(FILE *ms) {
    
    for(int i = 0; i < MaxBlocks; i ++) {
        Ms[i].address = i;
        Ms[i].free = true;
    }

    size_t fwrite(Ms, sizeof(BlockState), MaxBlocks, ms);

}

int main() {

}