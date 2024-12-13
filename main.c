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
    int next;
    bool isFree;
} block, buffer;

typedef struct {
    char fileName[40];
    int fileId;
    int sizeBlocks;
    int sizeEnrgs;
    int addressFirst;
    bool modeOrganizationGlobal;
    bool modeOrganizationIntern;
} metaData;

typedef struct {
    int fileId;
} file;

typedef struct {
    int address;
    bool free;
} BlockState;

typedef struct {
    metaData filesArray[MaxBlocks];
    int numberOf_files;
} FilesMeta;

void initialisationDisque(char fileName[]) {
    BlockState Ms[MaxBlocks];
    buffer buf;
    FilesMeta metas;
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

    metas.numberOf_files = 0;
    fwrite(&metas, sizeof(FilesMeta), 1, ms);

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

void tableDallocation(char fileName[], char operation, int add) {
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

void compactage(char fileName[]) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    BlockState blockStates[MaxBlocks];
    FilesMeta filesMeta;
    fseek(ms, 0, SEEK_SET);
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    block msBlocks[MaxBlocks];
    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta), SEEK_SET);
    fread(msBlocks, sizeof(block), MaxBlocks, ms);

    int destIndex = 0;
    for (int i = 0; i < MaxBlocks; i++) {
        if (msBlocks[i].nbEnregistrement != 0) {
            if (i != destIndex) {
                msBlocks[destIndex] = msBlocks[i];
                blockStates[destIndex] = blockStates[i];
                msBlocks[i].isFree = true;
                msBlocks[i].nbEnregistrement = 0;
                blockStates[i].free = true;
            }
            if (destIndex > 0) {
                msBlocks[destIndex - 1].address = destIndex;
            }
            destIndex++;
        }
    }

    for (int i = destIndex; i < MaxBlocks; i++) {
        memset(&msBlocks[i], 0, sizeof(block));
        blockStates[i].free = true;
    }

    fseek(ms, 0, SEEK_SET);
    fwrite(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta), SEEK_SET);
    fwrite(msBlocks, sizeof(block), MaxBlocks, ms);

    fclose(ms);

    updateFileMetadataAfterCompaction(fileName);
}

void updateFileMetadataAfterCompaction(char fileName[]) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    FilesMeta filesMeta;
    fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        int firstBlockAddress = filesMeta.filesArray[i].addressFirst;
        
        for (int j = 0; j < MaxBlocks; j++) {
            if (firstBlockAddress == j) {
                filesMeta.filesArray[i].addressFirst = j;
                break;
            }
        }
    }

    fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fclose(ms);
}

void vider(char fileName[]) {
    BlockState tableDallocation[MaxBlocks];
    buffer buf;
    FilesMeta metas;
    FILE *ms = fopen(fileName, "r+b");

    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    for (int i = 0; i < MaxBlocks; i++) {
        tableDallocation[i].address = i;
        tableDallocation[i].free = true;
    }

    fseek(ms, 0, SEEK_SET);
    fwrite(tableDallocation, sizeof(BlockState), MaxBlocks, ms);
    
    metas.numberOf_files = 0;
    fwrite(&metas, sizeof(FilesMeta), 1, ms);


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

void gestionDeStockage(char fileName[], int requiredBlocks) {
    BlockState blockStates[MaxBlocks];

    FILE *ms = fopen(fileName, "rb");
    if (ms == NULL) {
        perror("Error opening file for stockage management");
        return;
    }

    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fclose(ms);

    int maxfree = 0;
    int freeBlocks;
    for (int i = 0; i < MaxBlocks; i++) {
        if (blockStates[i].free) {
            freeBlocks++;
            if(maxfree < freeBlocks){
                maxfree = freeBlocks;
            }
        }else{
            freeBlocks = 0;
        }
    }

    if (maxfree >= requiredBlocks) {
        printf("Sufficient storage available: %d free blocks.\n", maxfree);
    } else {
        printf("Insufficient storage. Performing compactage...\n");

        compactage(fileName);


        //this step only to check if the comptage really did the necessary job -It can be removed-
        freeBlocks = 0;
        ms = fopen(fileName, "rb");
        fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
        fclose(ms);
        for (int i = 0; i < MaxBlocks; i++) {
            if (blockStates[i].free) {
                freeBlocks++;
            }
        }

        if (freeBlocks >= requiredBlocks) {
            printf("Storage available after compactage: %d free blocks.\n", freeBlocks);
        } else {
            printf("Storage full! Unable to allocate required space.\n");
        }
    }
}

//***************************FILE MANAGMENT************************************

// Function to create a new data file

int main() {

}