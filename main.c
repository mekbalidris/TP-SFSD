#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// ************************************************************
// ************************************************************

#define MaxBlocks 30

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
} block, buffer;

typedef struct {
    char fileName[40];
    int fileId;
    int sizeBlocks;
    int sizeEnrgs;
    int addressFirst;
    bool modeOrganizationGlobal; //chaine / contegu
    bool modeOrganizationIntern; // trier / !trier
} metaData;

typedef struct {
    int fileId;
} file;

typedef struct {
    int address;
    bool free; // true for free and false for not free
} BlockState;

char fileName[] = "Ms.dat";

// ***********************************************************************
// ***********************************************************************

void initialisationDisque(char fileName[]) {
    BlockState Ms[MaxBlocks];
    FILE *ms = fopen(fileName, "wb");

    if (ms == NULL) {
        perror("Error opening file for writing");
        return;
    }

    for (int i = 0; i < MaxBlocks; i++) {
        Ms[i].address = i;
        Ms[i].free = true;
    }

    size_t elements = fwrite(Ms, sizeof(BlockState), MaxBlocks, ms);
    if (elements != MaxBlocks) {
        perror("Error writing to file");
    } else {
        printf("Blocks State Table written successfully\n");
    }

    fclose(ms);
}

void tableDallocation(char operation, int add) {
    BlockState Ms[MaxBlocks];
    BlockState state;
    FILE *ms = fopen(fileName, "rb+");

    if (ms == NULL) {
        perror("Error opening file for reading/writing");
        return;
    }

    fseek(ms, sizeof(BlockState) * add, SEEK_SET);
    state.address = add;

    switch (operation) {
        case 'f': 
            state.free = true;
        break;
        
        case 'a': 
            state.free = false;
        break;
        
        default:
            printf("Invalid operation\n");
            fclose(ms);
        return;
    }

    fwrite(&state, sizeof(BlockState), 1, ms);
    fclose(ms);
}

int main() {

}