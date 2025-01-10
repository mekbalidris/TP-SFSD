#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MaxBlocks 30
#define BlockSize 3 // Maximum records per block

// Record structure
typedef struct {
    int id;
    char data[30];
    bool isDeleted;
} Record;

// Block structure
typedef struct {
    Record records[BlockSize];
    int nextBlock; // For chained organization
    int recordCount;
    bool isFree;
} Block;

// File metadata structure
typedef struct {
    char fileName[40];
    int fileId;
    int sizeBlocks;
    int sizeRecords;
    int firstBlock;
    bool isContiguous; // True for contiguous, False for chained
    bool isSorted;     // True for sorted, False for unsorted
} FileMeta;

// Secondary memory structure
typedef struct {
    Block blocks[MaxBlocks];
    FileMeta files[MaxBlocks];
    int fileCount;
    bool blockAllocation[MaxBlocks]; // True if block is free
} SecondaryMemory;

// Function prototypes
void initializeDisk(SecondaryMemory *disk);
void createFile(SecondaryMemory *disk);
void loadFile(SecondaryMemory *disk, int fileId);
void insertRecord(SecondaryMemory *disk, int fileId);
void searchRecord(SecondaryMemory *disk, int fileId, int recordId);
void deleteRecord(SecondaryMemory *disk, int fileId, int recordId, bool isPhysical);
void defragmentFile(SecondaryMemory *disk, int fileId);
void renameFile(SecondaryMemory *disk, int fileId, char *newName);
void deleteFile(SecondaryMemory *disk, int fileId);
void compactDisk(SecondaryMemory *disk);
void clearDisk(SecondaryMemory *disk);
void displayDiskState(SecondaryMemory *disk);
void displayMetadata(SecondaryMemory *disk);

// Main function
int main() {
    SecondaryMemory disk;
    initializeDisk(&disk);

    int choice;
    bool running = true;

    while (running) {
        printf("\n=== SECONDARY MEMORY MANAGEMENT SYSTEM ===\n");
        printf("1. Initialize Disk\n");
        printf("2. Create and Load File\n");
        printf("3. Insert Record\n");
        printf("4. Search Record\n");
        printf("5. Delete Record (Logical)\n");
        printf("6. Delete Record (Physical)\n");
        printf("7. Defragment File\n");
        printf("8. Rename File\n");
        printf("9. Delete File\n");
        printf("10. Compact Disk\n");
        printf("11. Clear Disk\n");
        printf("12. Display Disk State\n");
        printf("13. Display File Metadata\n");
        printf("14. Exit\n");
        printf("Choose an option (1-14): ");
        scanf("%d", &choice);

        int fileId, recordId;
        char newName[40];

        switch (choice) {
            case 1:
                initializeDisk(&disk);
                printf("Disk initialized successfully.\n");
                break;

            case 2:
                createFile(&disk);
                break;

            case 3:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                insertRecord(&disk, fileId);
                break;

            case 4:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                printf("Enter Record ID: ");
                scanf("%d", &recordId);
                searchRecord(&disk, fileId, recordId);
                break;

            case 5:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                printf("Enter Record ID: ");
                scanf("%d", &recordId);
                deleteRecord(&disk, fileId, recordId, false);
                break;

            case 6:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                printf("Enter Record ID: ");
                scanf("%d", &recordId);
                deleteRecord(&disk, fileId, recordId, true);
                break;

            case 7:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                defragmentFile(&disk, fileId);
                break;

            case 8:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                printf("Enter New Name: ");
                scanf("%s", newName);
                renameFile(&disk, fileId, newName);
                break;

            case 9:
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                deleteFile(&disk, fileId);
                break;

            case 10:
                compactDisk(&disk);
                break;

            case 11:
                clearDisk(&disk);
                break;

            case 12:
                displayDiskState(&disk);
                break;

            case 13:
                displayMetadata(&disk);
                break;

            case 14:
                running = false;
                printf("Exiting program...\n");
                break;

            default:
                printf("Invalid option. Please choose a number between 1 and 14.\n");
                break;
        }
    }

    return 0;
}

// Initialize the disk
void initializeDisk(SecondaryMemory *disk) {
    for (int i = 0; i < MaxBlocks; i++) {
        disk->blockAllocation[i] = true; // Mark all blocks as free
        disk->blocks[i].isFree = true;
        disk->blocks[i].recordCount = 0;
        disk->blocks[i].nextBlock = -1;
    }
    disk->fileCount = 0;
}

// Create a new file
void createFile(SecondaryMemory *disk) {
    if (disk->fileCount >= MaxBlocks) {
        printf("Maximum number of files reached!\n");
        return;
    }

    FileMeta newFile;
    printf("Enter file name: ");
    scanf("%s", newFile.fileName);
    printf("Enter number of records: ");
    scanf("%d", &newFile.sizeRecords);
    printf("Choose organization (0 for contiguous, 1 for chained): ");
    scanf("%d", &newFile.isContiguous);
    printf("Choose sorting (0 for unsorted, 1 for sorted): ");
    scanf("%d", &newFile.isSorted);

    // Calculate required blocks
    int requiredBlocks = ceil((double)newFile.sizeRecords / BlockSize);
    int freeBlocks = 0;
    for (int i = 0; i < MaxBlocks; i++) {
        if (disk->blockAllocation[i]) freeBlocks++;
    }

    if (freeBlocks < requiredBlocks) {
        printf("Not enough free blocks. Required: %d, Available: %d\n", requiredBlocks, freeBlocks);
        return;
    }

    // Allocate blocks
    newFile.firstBlock = -1;
    int prevBlock = -1;
    for (int i = 0; i < MaxBlocks && requiredBlocks > 0; i++) {
        if (disk->blockAllocation[i]) {
            disk->blockAllocation[i] = false;
            disk->blocks[i].isFree = false;
            if (newFile.firstBlock == -1) newFile.firstBlock = i;
            if (prevBlock != -1) disk->blocks[prevBlock].nextBlock = i;
            prevBlock = i;
            requiredBlocks--;
        }
    }

    newFile.sizeBlocks = ceil((double)newFile.sizeRecords / BlockSize);
    newFile.fileId = disk->fileCount;
    disk->files[disk->fileCount++] = newFile;

    printf("File created successfully with ID: %d\n", newFile.fileId);
}

// Insert a record into a file
void insertRecord(SecondaryMemory *disk, int fileId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("Invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    Record newRecord;
    printf("Enter record ID: ");
    scanf("%d", &newRecord.id);
    printf("Enter record data: ");
    scanf("%s", newRecord.data);
    newRecord.isDeleted = false;

    int currentBlock = file->firstBlock;
    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        if (block->recordCount < BlockSize) {
            block->records[block->recordCount++] = newRecord;
            file->sizeRecords++;
            printf("Record inserted successfully.\n");
            return;
        }
        currentBlock = block->nextBlock;
    }

    printf("No space available in the file!\n");
}

// Search for a record by ID
void searchRecord(SecondaryMemory *disk, int fileId, int recordId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("Invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;
    int blockNumber = 0;

    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        for (int i = 0; i < block->recordCount; i++) {
            if (block->records[i].id == recordId && !block->records[i].isDeleted) {
                printf("Record found in Block %d, Position %d.\n", blockNumber, i);
                return;
            }
        }
        currentBlock = block->nextBlock;
        blockNumber++;
    }

    printf("Record not found!\n");
}

// Delete a record (logical or physical)
void deleteRecord(SecondaryMemory *disk, int fileId, int recordId, bool isPhysical) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("Invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;

    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        for (int i = 0; i < block->recordCount; i++) {
            if (block->records[i].id == recordId && !block->records[i].isDeleted) {
                if (isPhysical) {
                    // Shift records to remove the deleted record
                    for (int j = i; j < block->recordCount - 1; j++) {
                        block->records[j] = block->records[j + 1];
                    }
                    block->recordCount--;
                    file->sizeRecords--;
                } else {
                    block->records[i].isDeleted = true;
                }
                printf("Record deleted successfully.\n");
                return;
            }
        }
        currentBlock = block->nextBlock;
    }

    printf("Record not found!\n");
}

// Defragment a file
void defragmentFile(SecondaryMemory *disk, int fileId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("Invalid file ID!\n");
        return;
    }

    FileMeta *file = &disk->files[fileId];
    int currentBlock = file->firstBlock;
    int prevBlock = -1;

    while (currentBlock != -1) {
        Block *block = &disk->blocks[currentBlock];
        int nextBlock = block->nextBlock;

        // Remove deleted records
        int writeIndex = 0;
        for (int i = 0; i < block->recordCount; i++) {
            if (!block->records[i].isDeleted) {
                block->records[writeIndex++] = block->records[i];
            }
        }
        block->recordCount = writeIndex;

        // If block is empty, free it
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

    printf("File defragmented successfully.\n");
}

// Rename a file
void renameFile(SecondaryMemory *disk, int fileId, char *newName) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("Invalid file ID!\n");
        return;
    }

    strcpy(disk->files[fileId].fileName, newName);
    printf("File renamed successfully.\n");
}

// Delete a file
void deleteFile(SecondaryMemory *disk, int fileId) {
    if (fileId < 0 || fileId >= disk->fileCount) {
        printf("Invalid file ID!\n");
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

    // Shift files to remove the deleted file
    for (int i = fileId; i < disk->fileCount - 1; i++) {
        disk->files[i] = disk->files[i + 1];
    }
    disk->fileCount--;

    printf("File deleted successfully.\n");
}

// Compact the disk
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

    printf("Disk compacted successfully.\n");
}

// Clear the disk
void clearDisk(SecondaryMemory *disk) {
    initializeDisk(disk);
    printf("Disk cleared successfully.\n");
}

// Display disk state
void displayDiskState(SecondaryMemory *disk) {
    printf("\n=== DISK STATE ===\n");
    for (int i = 0; i < MaxBlocks; i++) {
        if (disk->blockAllocation[i]) {
            printf("[FREE] ");
        } else {
            printf("[USED] ");
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

// Display file metadata
void displayMetadata(SecondaryMemory *disk) {
    printf("\n=== FILE METADATA ===\n");
    printf("%-20s %-8s %-12s %-12s %-15s %-15s %-15s\n",
           "Filename", "FileID", "Total Blocks", "Total Records", "First Block", "Organization", "Sorting");
    printf("--------------------------------------------------------------------------------\n");
    for (int i = 0; i < disk->fileCount; i++) {
        printf("%-20s %-8d %-12d %-12d %-15d %-15s %-15s\n",
               disk->files[i].fileName,
               disk->files[i].fileId,
               disk->files[i].sizeBlocks,
               disk->files[i].sizeRecords,
               disk->files[i].firstBlock,
               disk->files[i].isContiguous ? "Contiguous" : "Chained",
               disk->files[i].isSorted ? "Sorted" : "Unsorted");
    }
    printf("\n");
}