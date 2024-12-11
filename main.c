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
// ****************initialization ********************
void initialisationDisque(char fileName[]) {
    BlockState Ms[MaxBlocks];
    buffer buf;
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
        perror("Error writing block state table to file");
        fclose(ms);
        return;
    }

    printf("Blocks State Table written successfully\n");

    for (int i = 0; i < MaxBlocks; i++) {
        memset(&buf, 0, sizeof(buf));
        buf.address = i;
        buf.nbEnregistrement = 0;
        buf.isFree = true;

        size_t contentWritten = fwrite(&buf, sizeof(buffer), 1, ms);
        if (contentWritten != 1) {
            perror("Error writing disk block content to file");
            fclose(ms);
            return;
        }
    }

    fclose(ms);
}


// ***************************************************************
// ***************************************************************
// *****************tableDallocation *****************************


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
// ************************************************************
// ************************************************************
// ************* compactage:

void compactage(char fileName[]){
    FILE *ms = fopen(fileName, "r+b");
    if(ms == NULL){
        perror("error opening file");
        return;
    }

    BlockState blockStates[MaxBlocks];
    buffer buff, compactedBuffer;

    fseek(ms, 0, SEEK_SET);
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);

    compactedBuffer.address = 0;
    compactedBuffer.nbEnregistrement = 0;
    compactedBuffer.isFree = false;

    int compactedBlockIndex = 0;

    for (int i = 0; i < MaxBlocks; i++) {
        if (!blockStates[i].free) {
            // Read the current block
            fseek(ms, sizeof(BlockState) + i * sizeof(buffer), SEEK_SET);
            fread(&buff, sizeof(buffer), 1, ms);

            // Traverse records in the block and move non-deleted ones
            for (int j = 0; j < buff.nbEnregistrement; j++) {
                if (!buff.tab[j].isDeleted) {
                    compactedBuffer.tab[compactedBuffer.nbEnregistrement] = buff.tab[j];
                    compactedBuffer.nbEnregistrement++;

                    // If the compacted buffer is full, write it and reset
                    if (compactedBuffer.nbEnregistrement == 3) {
                        fseek(ms, sizeof(BlockState) + compactedBlockIndex * sizeof(buffer), SEEK_SET);
                        fwrite(&compactedBuffer, sizeof(buffer), 1, ms);

                        compactedBlockIndex++;
                        memset(&compactedBuffer, 0, sizeof(buffer));
                        compactedBuffer.address = compactedBlockIndex;
                        compactedBuffer.nbEnregistrement = 0;
                        compactedBuffer.isFree = false;
                    }
                }
            }
        }
    }

    //write the remaining recors in compacted buffer if any

    if(compactedBuffer.nbEnregistrement > 0){
        fseek(ms, sizeof(BlockState) + compactedBlockIndex * sizeof(buffer), SEEK_SET);
        fwrite(&compactedBuffer, sizeof(buffer), 1, ms);
        compactedBlockIndex++;
    }

     // Update block states: mark unused blocks as free
    for (int i = compactedBlockIndex; i < MaxBlocks; i++) {
        blockStates[i].free = true;
        fseek(ms, sizeof(BlockState) + i * sizeof(buffer), SEEK_SET);
        memset(&buff, 0, sizeof(buffer));
        buff.address = i;
        buff.nbEnregistrement = 0;
        buff.isFree = true;
        fwrite(&buff, sizeof(buffer), 1, ms);
    }

    // Update block state table
    fseek(ms, 0, SEEK_SET);
    fwrite(blockStates, sizeof(BlockState), MaxBlocks, ms);

    fclose(ms);
    printf("Disk compacted successfully.\n");
}



// ************************************************************
// ************************************************************
// *************vidange de la memoire centrale ***************

void vider(char fileName[]) {
    BlockState tableDallocation[MaxBlocks];
    buffer buf;
    FILE *ms = fopen(fileName, "r+b");

    if (ms == NULL) {
        perror("Error opening file ");
        return;
    }

    for (int i = 0; i < MaxBlocks; i++) {
        tableDallocation[i].address = i;
        tableDallocation[i].free = true;
    }

    fseek(ms, 0, SEEK_SET);
    fwrite(tableDallocation, sizeof(BlockState), MaxBlocks, ms);

    for (int i = 0; i < MaxBlocks; i++) {
        memset(&buf, 0, sizeof(buffer));
        buf.address = i;
        buf.nbEnregistrement = 0;
        buf.isFree = true;

        fseek(ms, sizeof(BlockState) + sizeof(buffer) * i, SEEK_SET);
        fwrite(&buf, sizeof(buffer), 1, ms);
    }

    fclose(ms);
}

//***************************FILE MANAGMENT************************************

// Function to create a new data file

int main() {

}