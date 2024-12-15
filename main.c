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

//function to perform compaction on the disk
void compactage(char fileName[]) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    BlockState blockStates[MaxBlocks];
    FilesMeta filesMeta;

    //read block states and file metadata
    fseek(ms, 0, SEEK_SET);
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    block msBlocks[MaxBlocks];

    //read the blocks from the file
    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta), SEEK_SET);
    fread(msBlocks, sizeof(block), MaxBlocks, ms);

    int destIndex = 0;        //pointer to place the next valid block
    int lastValidBlock = -1;  //tracks the last valid block
    int currentFile = 0;      //tracks the current file being processed

    //iterate over each block to compact and move non-empty blocks to the front
    for (int i = 0; i < MaxBlocks; i++) {
        if (msBlocks[i].nbEnregistrement != 0) {
            //check if the current block belongs to a new file and update the 'next' pointer
            if (i != filesMeta.filesArray[currentFile].addressFirst &&
                (currentFile + 1 < filesMeta.numberOf_files) && 
                i == filesMeta.filesArray[currentFile + 1].addressFirst) {
                //update 'next' pointer to link the last valid block to the current block
                if (lastValidBlock != -1) {
                    msBlocks[lastValidBlock].next = i;
                }
                currentFile++;  //move to the next file
            }

            //move current block to the 'destIndex' position
            msBlocks[destIndex] = msBlocks[i];
            blockStates[destIndex] = blockStates[i];

            //mark the current block as free
            msBlocks[i].isFree = true;
            msBlocks[i].nbEnregistrement = 0;
            blockStates[i].free = true;

            //update the last valid block and move the destination index
            lastValidBlock = destIndex;
            destIndex++;
        }
    }

    //make all remaining blocks available and reset them as free
    for (int i = destIndex; i < MaxBlocks; i++) {
        memset(&msBlocks[i], 0, sizeof(block));  //clear the block
        blockStates[i].free = true;  //mark as free
        msBlocks[i].next = -1;       //set 'next' to -1 for empty blocks
    }

    //write the updated block states and blocks back to the file
    fseek(ms, 0, SEEK_SET);
    fwrite(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta), SEEK_SET);
    fwrite(msBlocks, sizeof(block), MaxBlocks, ms);

    fclose(ms);

    //update the file metadata after compaction
    updateFileMetadataAfterCompaction(fileName);
}

//function to update file metadata after compaction
void updateFileMetadataAfterCompaction(char fileName[]) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    BlockState blockStates[MaxBlocks];
    FilesMeta filesMeta;
    block msBlocks[MaxBlocks];

    //read block states, file metadata, and blocks from the file
    fseek(ms, 0, SEEK_SET);
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);
    fread(msBlocks, sizeof(block), MaxBlocks, ms);

    //update the 'addressFirst' field for each file after compaction
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        int currentBlock = filesMeta.filesArray[i].addressFirst;
        int previousBlock = -1;

        //traverse the blocks to find the last valid block in the file
        //curretBlock != -1 checks to see that the block isnt associated to any file
        while (currentBlock != -1 && msBlocks[currentBlock].nbEnregistrement != 0) {
            previousBlock = currentBlock;
            currentBlock = msBlocks[currentBlock].next;  //move to the next block in the chain
        }

        //if a valid block was found, update the 'addressFirst' of the file
        if (previousBlock != -1) {
            filesMeta.filesArray[i].addressFirst = previousBlock;
        }
    }

    //write the updated metadata and blocks back to the file
    fseek(ms, 0, SEEK_SET);
    fwrite(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);
    fwrite(msBlocks, sizeof(block), MaxBlocks, ms);

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

    //read the block states into memory
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fclose(ms);

    int maxfree = 0;
    int freeBlocks = 0; //initialize freeBlocks variable
    for (int i = 0; i < MaxBlocks; i++) {
        if (blockStates[i].free) {
            freeBlocks++;
            if (maxfree < freeBlocks) {
                maxfree = freeBlocks;
            }
        } else {
            freeBlocks = 0; //reset freeBlocks count when a non-free block is found
        }
    }

    //check if there are enough free blocks
    if (maxfree >= requiredBlocks) {
        printf("Sufficient storage available: %d free blocks.\n", maxfree);
    } else {
        printf("Insufficient storage. Performing compactage...\n");

        //call compactage to defragment the storage
        compactage(fileName);

        //re-check the number of free blocks after compaction
        freeBlocks = 0;
        ms = fopen(fileName, "rb");
        if (ms == NULL) {
            perror("Error opening file after compaction");
            return;
        }
        fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
        fclose(ms);

        for (int i = 0; i < MaxBlocks; i++) {
            if (blockStates[i].free) {
                freeBlocks++;
            }
        }

        //after compaction, check if there are enough free blocks
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