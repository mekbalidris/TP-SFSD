#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MaxBlocks 30          // total number of blocks in the disk
#define BlockSize 3           // maximum number of records each block can hold

// structure to represent a single record
typedef struct {
    int id;                   // unique identifier for the record
    char data[30];            // data stored in the record
    bool isDeleted;           // flag to mark if the record is deleted (for logical deletion)
} Record;

// structure to represent a block of records
typedef struct {
    Record records[BlockSize]; // array to store records in the block
    int nextBlock;             // pointer to the next block in the chain (for chained files)
    int recordCount;           // number of records currently stored in the block
    bool isFree;               // flag to indicate if the block is free or in use
} Block;

// structure to store metadata about a file
typedef struct {
    char fileName[40];         // name of the file
    int fileId;                // unique identifier for the file
    int sizeBlocks;            // number of blocks allocated to the file
    int sizeRecords;           // total number of records in the file
    int firstBlock;            // pointer to the first block of the file
    bool isContiguous;         // true if the file is stored contiguously, false if chained
    bool isSorted;             // true if the file is sorted, false if unsorted
} FileMeta;

// structure to represent the entire disk
typedef struct {
    Block blocks[MaxBlocks];   // array of all blocks in the disk
    FileMeta files[MaxBlocks]; // array to store metadata for all files
    int fileCount;             // number of files currently stored on the disk
    bool blockAllocation[MaxBlocks]; // array to track which blocks are free (true = free)
} SecondaryMemory;

// function prototypes
void initializeDisk(SecondaryMemory *disk); //moh
void createFile(SecondaryMemory *disk); //idris
void loadFile(SecondaryMemory *disk, int fileId); //moh
//void insertRecord(SecondaryMemory *disk, int fileId);                                   //Madjda
void searchRecord(SecondaryMemory *disk, int fileId, int recordId); //idris
void deleteRecord(SecondaryMemory *disk, int fileId, int recordId, bool isPhysical); //mouhaimen
//void defragmentFile(SecondaryMemory *disk, int fileId);                                 //Madjda
void renameFile(SecondaryMemory *disk, int fileId, char *newName); // mouhaimen
void deleteFile(SecondaryMemory *disk, int fileId); //youcef
//void compactDisk(SecondaryMemory *disk);                                                //Madjda
void clearDisk(SecondaryMemory *disk); //youcef
void displayDiskState(SecondaryMemory *disk); //youcef
void displayMetadata(SecondaryMemory *disk);  //youcef

// main function to run the program
int main() {
    SecondaryMemory disk;
    initializeDisk(&disk);  // start by initializing the disk

    int choice;
    bool running = true;

    while (running) {
        // display the menu for the user
        printf("\n=== SECONDARY MEMORY MANAGEMENT SYSTEM ===\n");
        printf("1. initialize disk\n");
        printf("2. create and load file\n");
        printf("3. insert record\n");
        printf("4. search record\n");
        printf("5. delete record (logical)\n");
        printf("6. delete record (physical)\n");
        printf("7. defragment file\n");
        printf("8. rename file\n");
        printf("9. delete file\n");
        printf("10. compact disk\n");
        printf("11. clear disk\n");
        printf("12. display disk state\n");
        printf("13. display file metadata\n");
        printf("14. exit\n");
        printf("choose an option (1-14): ");
        scanf("%d", &choice);

        int fileId, recordId;
        char newName[40];

        switch (choice) {
            case 1:
                initializeDisk(&disk);
                printf("disk initialized successfully.\n");
                break;

            case 2:
                createFile(&disk);  // create a new file
                break;

            case 3:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                insertRecord(&disk, fileId);  // insert a record into the file
                break;

            case 4:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                printf("enter record ID: ");
                scanf("%d", &recordId);
                searchRecord(&disk, fileId, recordId);  // search for a record
                break;

            case 5:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                printf("enter record ID: ");
                scanf("%d", &recordId);
                deleteRecord(&disk, fileId, recordId, false);  // logically delete a record
                break;

            case 6:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                printf("enter record ID: ");
                scanf("%d", &recordId);
                deleteRecord(&disk, fileId, recordId, true);  // physically delete a record
                break;

            case 7:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                defragmentFile(&disk, fileId);  // defragment the file
                break;

            case 8:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                printf("enter new name: ");
                scanf("%s", newName);
                renameFile(&disk, fileId, newName);  // rename the file
                break;

            case 9:
                printf("enter file ID: ");
                scanf("%d", &fileId);
                deleteFile(&disk, fileId);  // delete the file
                break;

            case 10:
                compactDisk(&disk);  // compact the disk to free up space
                break;

            case 11:
                clearDisk(&disk);  // clear the entire disk
                break;

            case 12:
                displayDiskState(&disk);  // show the current state of the disk
                break;

            case 13:
                displayMetadata(&disk);  // display metadata for all files
                break;

            case 14:
                running = false;  // exit the program
                printf("exiting program...\n");
                break;

            default:
                printf("invalid option. please choose a number between 1 and 14.\n");
                break;
        }
    }

    return 0;
}

// function to initialize the disk
void initializeDisk(SecondaryMemory *disk) {
    for (int i = 0; i < MaxBlocks; i++) {
        disk->blockAllocation[i] = true; // mark all blocks as free
        disk->blocks[i].isFree = true;
        disk->blocks[i].recordCount = 0;
        disk->blocks[i].nextBlock = -1;  // no next block initially
    }
    disk->fileCount = 0;  // no files on the disk yet
}

// function to create a new file
void createFile(SecondaryMemory *disk) {
    // check if the maximum number of files has been reached
    if (disk->fileCount >= MaxBlocks) {
        printf("maximum number of files reached!\n");
        return;
    }

    // create a new file and get its details from the user
    FileMeta newFile;
    printf("enter file name: ");
    scanf("%s", newFile.fileName);
    printf("enter number of records: ");
    scanf("%d", &newFile.sizeRecords);
    printf("choose organization (0 for contiguous, 1 for chained): ");
    scanf("%d", &newFile.isContiguous);
    printf("choose sorting (0 for unsorted, 1 for sorted): ");
    scanf("%d", &newFile.isSorted);

    // calculate how many blocks are needed for the file
    int requiredBlocks = ceil((double)newFile.sizeRecords / BlockSize);

    // check how many free blocks are available on the disk
    int freeBlocks = 0;
    for (int i = 0; i < MaxBlocks; i++) {
        if (disk->blockAllocation[i]) freeBlocks++;
    }

    // if there aren't enough free blocks, print an error and exit
    if (freeBlocks < requiredBlocks) {
        printf("not enough free blocks. required: %d, available: %d\n", requiredBlocks, freeBlocks);
        return;
    }

    // initialize file metadata
    newFile.firstBlock = -1; // No blocks allocated yet
    newFile.sizeBlocks = requiredBlocks;
    newFile.fileId = disk->fileCount;

    // allocate blocks based on the file's organization (contiguous or chained)
    int prevBlock = -1; // Tracks the last block allocated to the file

    if (newFile.isContiguous) {
        // try to find contiguous blocks for contiguous allocation
        bool foundContiguous = false;
        for (int i = 0; i <= MaxBlocks - requiredBlocks; i++) {
            bool isContiguousFree = true;
            // check if the next `requiredBlocks` blocks are free
            for (int j = i; j < i + requiredBlocks; j++) {
                if (!disk->blockAllocation[j]) {
                    isContiguousFree = false;
                    break;
                }
            }
            // if contiguous blocks are found, allocate them
            if (isContiguousFree) {
                for (int j = i; j < i + requiredBlocks; j++) {
                    disk->blockAllocation[j] = false; // mark block as used
                    disk->blocks[j].isFree = false;   // mark block as used
                    if (newFile.firstBlock == -1) newFile.firstBlock = j; // set first block
                    if (prevBlock != -1) disk->blocks[prevBlock].nextBlock = j; // link blocks
                    prevBlock = j;
                }
                foundContiguous = true;
                break;
            }
        }
        // if no contiguous blocks are found, print an error and exit
        if (foundContiguous == false) {
            printf("not enough contiguous blocks available!\n");
            return;
        }
    } else {
        // allocate blocks in a chained manner (any free blocks)
        for (int i = 0; i < MaxBlocks && requiredBlocks > 0; i++) {
            if (disk->blockAllocation[i]) {
                disk->blockAllocation[i] = false; // mark block as used
                disk->blocks[i].isFree = false;   // mark block as used
                if (newFile.firstBlock == -1) newFile.firstBlock = i; // set first block
                if (prevBlock != -1) disk->blocks[prevBlock].nextBlock = i; // link blocks
                prevBlock = i;
                requiredBlocks--;
            }
        }
    }

    // add the new file to the disk's file list
    disk->files[disk->fileCount++] = newFile;

    // print success message
    printf("file created successfully with ID: %d\n", newFile.fileId);
}

// function to insert a record into a file
void insertRecord(SecondaryMemory *disk, int fileId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    Record newRecord;
    printf("enter record ID: ");
    scanf("%d", &newRecord.id);
    printf("enter record data: ");
    scanf("%s", newRecord.data);
    newRecord.isDeleted = false;

    int currentBlock = file->firstBlock;
    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        if (block->recordCount < BlockSize) {
            block->records[block->recordCount++] = newRecord;
            file->sizeRecords++;
            printf("record inserted successfully.\n");
            return;
        }
        currentBlock = block->nextBlock;
    }

    printf("no space available in the file!\n");
}

// function to search for a record by ID
void searchRecord(SecondaryMemory *disk, int fileId, int recordId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;
    int blockNumber = 0;

    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        for (int i = 0; i < block->recordCount; i++) {
            if (block->records[i].id == recordId && !block->records[i].isDeleted) {
                printf("record found in block %d, position %d.\n", blockNumber, i);
                return;
            }
        }
        currentBlock = block->nextBlock;
        blockNumber++;
    }

    printf("record not found!\n");
}

// function to delete a record (logically or physically)
void deleteRecord(SecondaryMemory *disk, int fileId, int recordId, bool isPhysical) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;

    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        for (int i = 0; i < block->recordCount; i++) {
            if (block->records[i].id == recordId && !block->records[i].isDeleted) {
                if (isPhysical) {
                    // shift records to remove the deleted record
                    for (int j = i; j < block->recordCount - 1; j++) {
                        block->records[j] = block->records[j + 1];
                    }
                    block->recordCount--;
                    file->sizeRecords--;
                } else {
                    block->records[i].isDeleted = true;
                }
                printf("record deleted successfully.\n");
                return;
            }
        }
        currentBlock = block->nextBlock;
    }

    printf("record not found!\n");
}

// function to defragment a file by removing deleted records and freeing empty blocks
void defragmentFile(SecondaryMemory *disk, int fileId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;
    int prevBlock = -1;

    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        int nextBlock = block->nextBlock;

        // remove deleted records from the block
        int writeIndex = 0;
        for (int i = 0; i < block->recordCount; i++) {
            if (!block->records[i].isDeleted) {
                block->records[writeIndex++] = block->records[i];
            }
        }
        block->recordCount = writeIndex;

        // if the block is empty after defragmentation, free it
        if (block->recordCount == 0) {
            disk->blockAllocation[currentBlock] = true;
            block->isFree = true;
            if (prevBlock != -1) {
                disk->blocks[prevBlock].nextBlock = nextBlock;
            } else {
                file->firstBlock = nextBlock;
            }
        } else {
            prevBlock = currentBlock;
        }

        currentBlock = nextBlock;
    }

    printf("file defragmented successfully.\n");
}

// function to rename a file
void renameFile(SecondaryMemory *disk, int fileId, char *newName) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("invalid file ID!\n");
        return;
    }

    strcpy(disk->files[fileId].fileName, newName);
    printf("file renamed successfully.\n");
}

// function to delete a file and free its blocks
void deleteFile(SecondaryMemory *disk, int fileId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;

    while (currentBlock != -1) {
        disk->blockAllocation[currentBlock] = true;
        disk->blocks[currentBlock].isFree = true;
        disk->blocks[currentBlock].recordCount = 0;
        int nextBlock = disk->blocks[currentBlock].nextBlock;
        disk->blocks[currentBlock].nextBlock = -1;
        currentBlock = nextBlock;
    }

    // shift files to remove the deleted file
    for (int i = fileId; i < disk->fileCount - 1; i++) {
        disk->files[i] = disk->files[i + 1];
    }
    disk->fileCount--;

    printf("file deleted successfully.\n");
}

// function to compact the disk by moving all used blocks to the front
void compactDisk(SecondaryMemory *disk) {
    int freeIndex = 0;
    for (int i = 0; i < MaxBlocks; i++) {
        if (disk->blockAllocation[i]) {
            freeIndex = i;
            break;
        }
    }

    for (int i = 0; i < MaxBlocks; i++) {
        if (!disk->blockAllocation[i]) {
            disk->blocks[freeIndex] = disk->blocks[i];
            disk->blockAllocation[freeIndex] = false;
            disk->blockAllocation[i] = true;
            freeIndex++;
        }
    }

    printf("disk compacted successfully.\n");
}

// function to clear the entire disk and reset it
void clearDisk(SecondaryMemory *disk) {
    initializeDisk(disk);
    printf("disk cleared successfully.\n");
}

// function to display the current state of the disk
void displayDiskState(SecondaryMemory *disk) {
    printf("\n=== disk state ===\n");
    for (int i = 0; i < MaxBlocks; i++) {
        if (disk->blockAllocation[i]) {
            printf("[free] ");
        } else {
            printf("[used] ");
            for (int j = 0; j < disk->fileCount; j++) {
                if (disk->files[j].firstBlock == i) {
                    printf("'%s' (%d records) ", disk->files[j].fileName, disk->blocks[i].recordCount);
                    break;
                }
            }
        }
        if ((i + 1) % 5 == 0) printf("\n");
    }
    printf("\n");
}

// function to display metadata for all files
void displayMetadata(SecondaryMemory *disk) {
    printf("\n=== file metadata ===\n");
    printf("%-20s %-8s %-12s %-12s %-15s %-15s %-15s\n",
           "filename", "fileID", "total blocks", "total records", "first block", "organization", "sorting");
    printf("--------------------------------------------------------------------------------\n");
    for (int i = 0; i < disk->fileCount; i++) {
        printf("%-20s %-8d %-12d %-12d %-15d %-15s %-15s\n",
               disk->files[i].fileName,
               disk->files[i].fileId,
               disk->files[i].sizeBlocks,
               disk->files[i].sizeRecords,
               disk->files[i].firstBlock,
               disk->files[i].isContiguous ? "contiguous" : "chained",
               disk->files[i].isSorted ? "sorted" : "unsorted");
    }
    printf("\n");
}