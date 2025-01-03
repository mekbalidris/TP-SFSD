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
    fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
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

    fseek(ms, add * sizeof(BlockState), SEEK_SET);
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
    FILE *ms = fopen(fileName, "rb+"); // Open the file in read-write mode
    if (ms == NULL) {
        perror("Error opening file"); // Display error if file cannot be opened
        return;
    }

    BlockState blockStates[MaxBlocks]; // Array to store block states
    FilesMeta filesMeta;

    // Read block states and file metadata from the file
    fseek(ms, 0, SEEK_SET); // Move the file pointer to the beginning
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    block msBlocks[MaxBlocks]; // Array to store all blocks

    // Read all blocks from the file
    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta), SEEK_SET);
    fread(msBlocks, sizeof(block), MaxBlocks, ms);

    int destIndex = 0;        // Pointer to place the next valid block
    int lastValidBlock = -1;  // Tracks the last valid block encountered
    int currentFile = 0;      // Tracks the current file being processed

    // Iterate through all blocks
    for (int i = 0; i < MaxBlocks; i++) {
        if (msBlocks[i].nbEnregistrement != 0) { // Check if block is not empty

            // Check if the block starts a new file and update its pointers
            if (i != filesMeta.filesArray[currentFile].addressFirst &&
                (currentFile + 1 < filesMeta.numberOf_files) &&
                i == filesMeta.filesArray[currentFile + 1].addressFirst) {

                // Update 'next' pointer of the previous block
                if (lastValidBlock != -1) {
                    msBlocks[lastValidBlock].next = i;
                }
                currentFile++;  // Move to the next file
            }

            // Move the block to the destination index
            msBlocks[destIndex] = msBlocks[i];
            blockStates[destIndex] = blockStates[i];

            // Mark the current block as free
            msBlocks[i].isFree = true;
            msBlocks[i].nbEnregistrement = 0;
            blockStates[i].free = true;

            // Update last valid block and destination index
            lastValidBlock = destIndex;
            destIndex++;
        }
    }

    // Reset all remaining blocks to free
    for (int i = destIndex; i < MaxBlocks; i++) {
        memset(&msBlocks[i], 0, sizeof(block));  // Clear the block's data
        blockStates[i].free = true;  // Mark block as free
        msBlocks[i].next = -1;       // Set the 'next' pointer to -1
    }

    // Write updated block states and blocks back to the file
    fseek(ms, 0, SEEK_SET);
    fwrite(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);
    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta), SEEK_SET);
    fwrite(msBlocks, sizeof(block), MaxBlocks, ms);

    fclose(ms); // Close the file

    // Update file metadata after compaction
    updateFileMetadataAfterCompaction(fileName);
}

//function to update file metadata after compaction
void updateFileMetadataAfterCompaction(char fileName[]) {
    FILE *ms = fopen(fileName, "rb+"); // Open the file in read-write mode
    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    BlockState blockStates[MaxBlocks];
    FilesMeta filesMeta;
    block msBlocks[MaxBlocks];

    // Read block states, metadata, and blocks
    fseek(ms, 0, SEEK_SET);
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);
    fread(msBlocks, sizeof(block), MaxBlocks, ms);

    // Update the 'addressFirst' field for each file
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        int currentBlock = filesMeta.filesArray[i].addressFirst;
        int previousBlock = -1;

        // Traverse the blocks to find the last valid block
        while (currentBlock != -1 && msBlocks[currentBlock].nbEnregistrement != 0) {
            previousBlock = currentBlock;
            currentBlock = msBlocks[currentBlock].next; // Move to the next block
        }

        // Update 'addressFirst' if a valid block was found
        if (previousBlock != -1) {
            filesMeta.filesArray[i].addressFirst = previousBlock;
        }
    }

    // Write the updated metadata and blocks back to the file
    fseek(ms, 0, SEEK_SET);
    fwrite(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);
    fwrite(msBlocks, sizeof(block), MaxBlocks, ms);

    fclose(ms); // Close the file
}

void vider(char fileName[]) {
    BlockState tableDallocation[MaxBlocks]; // Allocation table
    buffer buf;
    FilesMeta metas;
    FILE *ms = fopen(fileName, "r+b"); // Open file in read-write mode

    if (ms == NULL) {
        perror("Error opening file"); // Handle error
        return;
    }

    // Mark all blocks as free
    for (int i = 0; i < MaxBlocks; i++) {
        tableDallocation[i].address = i;
        tableDallocation[i].free = true;
    }

    fseek(ms, 0, SEEK_SET);
    fwrite(tableDallocation, sizeof(BlockState), MaxBlocks, ms);

    // Reset metadata
    metas.numberOf_files = 0;
    fwrite(&metas, sizeof(FilesMeta), 1, ms);

    // Clear each block
    for (int i = 0; i < MaxBlocks; i++) {
        memset(&buf, 0, sizeof(buffer));
        buf.address = i;
        buf.nbEnregistrement = 0;
        buf.isFree = true;

        fseek(ms, sizeof(BlockState) + sizeof(buffer) * i, SEEK_SET);
        fwrite(&buf, sizeof(buffer), 1, ms);
    }

    fclose(ms); // Close the file
}

void gestionDeStockage(char fileName[], int requiredBlocks) {
    BlockState blockStates[MaxBlocks];

    FILE *ms = fopen(fileName, "rb"); // Open the file in read mode
    if (ms == NULL) {
        perror("Error opening file for stockage management");
        return;
    }

    // Read block states
    fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
    fclose(ms);

    int maxfree = 0;
    int freeBlocks = 0; // Initialize the free block counter

    for (int i = 0; i < MaxBlocks; i++) {
        if (blockStates[i].free) {
            freeBlocks++;
            if (maxfree < freeBlocks) {
                maxfree = freeBlocks;
            }
        } else {
            freeBlocks = 0; // Reset counter when encountering a used block
        }
    }

    // Check for sufficient storage
    if (maxfree >= requiredBlocks) {
        printf("Sufficient storage available: %d free blocks.\n", maxfree);
    } else {
        printf("Insufficient storage. Performing compactage...\n");

        // Perform compaction
        compactage(fileName);

        // Re-check storage availability
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

        if (freeBlocks >= requiredBlocks) {
            printf("Storage available after compactage: %d free blocks.\n", freeBlocks);
        } else {
            printf("Storage full! Unable to allocate required space.\n");
        }
    }
}


//***************************FILE MANAGMENT************************************

// Function to create a new data file

void insertRecordChained(char fileName[], int fileId, enregistrement record) {
    FILE *ms = fopen(fileName, "rb+");
    if(ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState BlockState[MaxBlocks];
    FilesMeta filesMeta; 
    block currentBlock;

    fread(BlockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    metaData *fileMeta = NULL;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            fileMeta = &filesMeta.filesArray[i];
            break;
        }
    }

    if (fileMeta == NULL) {
        printf("File not found!\n");
        fclose(ms);
        return;
    }

    int currentBlockAddress = fileMeta->addressFirst;
    bool isSorted = fileMeta->modeOrganizationIntern;

    // Find the appropriate block for insertion
    while (currentBlockAddress != -1) {
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
        fread(&currentBlock, sizeof(block), 1, ms);
        
        if (currentBlock.nbEnregistrement < 3) {
            // Found space in current block
            int insertPos = currentBlock.nbEnregistrement;
            
            if (isSorted) {
                // Find correct position in sorted block
                insertPos = 0;
                while (insertPos < currentBlock.nbEnregistrement && 
                       currentBlock.tab[insertPos].id < record.id) {
                    insertPos++;
                }
                
                // Shift records to make space
                for (int i = currentBlock.nbEnregistrement; i > insertPos; i--) {
                    currentBlock.tab[i] = currentBlock.tab[i-1];
                }
            }
            
            // Insert the record
            currentBlock.tab[insertPos] = record;
            currentBlock.nbEnregistrement++;
            
            // Write back the updated block
            fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
            fwrite(&currentBlock, sizeof(block), 1, ms);
            
            fileMeta->sizeEnrgs++;
            
            // Update file metadata
            fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
            fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);
            
            fclose(ms);
            return;
        }
        
        currentBlockAddress = currentBlock.next;
    }

    // Need to create new block
    int newBlockAddress = -1;
    for (int i = 0; i < MaxBlocks; i++) {
        if (BlockState[i].free) {
            newBlockAddress = i;
            break;
        }
    }

    if (newBlockAddress == -1) {
        printf("No free blocks available!\n");
        fclose(ms);
        return;
    }

    // Initialize new block
    memset(&currentBlock, 0, sizeof(block));
    currentBlock.address = newBlockAddress;
    currentBlock.nbEnregistrement = 1;
    currentBlock.tab[0] = record;
    currentBlock.next = -1;
    currentBlock.isFree = false;

    // Update block allocation
    BlockState[newBlockAddress].free = false;
    tableDallocation(fileName, 'a', newBlockAddress);

    // Write new block
    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * newBlockAddress, SEEK_SET);
    fwrite(&currentBlock, sizeof(block), 1, ms);

    // Update file metadata
    fileMeta->sizeBlocks++;
    fileMeta->sizeEnrgs++;
    fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fclose(ms);
}

// physical delete function
void physicalDelete(char fileName[], int fileId, int recordId){
    FILE *ms = fopen(fileName, "rb+");
    if(ms == NULL){
        printf("Error opening file for reading/writing");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta; 
    block currentBlock;

    fread(blockState,sizeof(BlockState),MaxBlocks,ms); // load block state 
    fread(&filesMeta,sizeof(FilesMeta),1,ms); // load files metadata

    metaData *fileMeta = NULL; // pointer to the file metadata
    for(int i = 0;i<filesMeta.numberOf_files;i++){
        if(filesMeta.filesArray[i].fileId == fileId){
            fileMeta = &filesMeta.filesArray[i];
            break;
        }
    }
    if(fileMeta == NULL){
        printf("File not found\n");
        return;
    }

    int currentBlockAdress;
    int prevBlockAdress = -1; // assuming that we only have one block

    if(fileMeta->modeOrganizationGlobal){ // assuming true means 'chained'
        currentBlockAdress = fileMeta->addressFirst;  

        while(currentBlockAdress != -1){ 
    
            fseek(ms, sizeof(BlockState)*MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAdress, SEEK_SET); 
            fread(&currentBlock,sizeof(block),1,ms); // read current block data

            // search for record in th current block
            for(int i=0;i<currentBlock.nbEnregistrement;i++){
                if(currentBlock.tab[i].id == recordId){
                    for(int j=i;j<currentBlock.nbEnregistrement-1;j++){
                        currentBlock.tab[j] = currentBlock.tab[j+1]; // shift to the left
                    }
                    currentBlock.nbEnregistrement--;

                    if(currentBlock.nbEnregistrement==0){ // check if the block is empty
                        blockState[currentBlockAdress].free = true;
                        tableDallocation(fileName,'f',currentBlockAdress);

                        if(prevBlockAdress != -1){ // the current block is empty and it is located at either the middle or the end of the chained blocks of the file

                            // Update previous block's 'next' pointer
                            block previousBlock;
                            fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * prevBlockAdress, SEEK_SET);
                            fread(&previousBlock, sizeof(block), 1, ms);
                            
                            // sets the previous block as the final block if there aren't other blocks
                            previousBlock.next = currentBlock.next; // or links the previous block to the block that is after the current one (deallocating the current one bcz it's empty)
                            
                            //  Write the updated previous block back to the file
                            fseek(ms, -sizeof(block), SEEK_CUR);
                            fwrite(&previousBlock, sizeof(block), 1, ms);
                        }
                        else{ // the current block is empty and it's the first block of the file

                            // Update the file metadata to point to the next block
                            fileMeta->addressFirst = currentBlock.next;
                        }

                      fileMeta->sizeBlocks--; // if the block is empty then size of file in blocks decreases
                    }

                  fileMeta->sizeEnrgs--;  // if record was found and deleted then size of file in records decreases

                  // write updated block and metadata
                  fseek(ms, sizeof(BlockState)*MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAdress, SEEK_SET);
                  fwrite(&currentBlock,sizeof(block),1,ms);

                  //  presist on updating the global Blockstate table and the file metadata in the file ( saved in the virtual disk file ms)
                  // ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
                  fseek(ms,0,SEEK_SET); 
                  fwrite(blockState, sizeof(BlockState),MaxBlocks,ms); 
                  fwrite(&filesMeta,sizeof(filesMeta),1,ms); 

                  fclose(ms);
                  printf("record %d deleted succesfully \n");
                  return;
                }
            }
            prevBlockAdress = currentBlockAdress;
            currentBlockAdress = currentBlock.next;
        } 
    }
    else{
        for (int i = 0; i < fileMeta->sizeBlocks; i++) {
            currentBlockAdress = fileMeta->addressFirst + i;

            // Read the current block
            fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAdress, SEEK_SET);
            fread(&currentBlock, sizeof(block), 1, ms);

            // Search for the record in the block
            for (int j = 0; j < currentBlock.nbEnregistrement; j++) {
                if (currentBlock.tab[j].id == recordId) {
                    // Shift remaining records
                    for (int k = j; k < currentBlock.nbEnregistrement - 1; k++) {
                        currentBlock.tab[k] = currentBlock.tab[k + 1];
                    }
                    currentBlock.nbEnregistrement--;

                    // Check if block is empty
                    if (currentBlock.nbEnregistrement == 0) {
                        blockState[currentBlockAdress].free = true;
                        tableDallocation(fileName, 'f', currentBlockAdress);
                        fileMeta->sizeBlocks--;
                    }

                    fileMeta->sizeEnrgs--;

                    // Write updated block and metadata
                    fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAdress, SEEK_SET);
                    fwrite(&currentBlock, sizeof(block), 1, ms);
                    // same logic as in chained
                    // ????????????????????????????????????????????????????????????????????????????????????????????????????????????,
                    fseek(ms, 0, SEEK_SET);
                    fwrite(blockState, sizeof(BlockState), MaxBlocks, ms);
                    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

                    fclose(ms);
                    printf("Record %d deleted successfully\n", recordId);
                    return;
                }
            }
        }
    }
    printf("Record %d not found in file %d!\n", recordId, fileId);
    fclose(ms);
} 


// Function to input records into blocks and save them into a file
// Parameters:
// f - file pointer where the blocks will be written
// nbrREC - total number of records to be added
// a - block number where the writing starts
void saisir(FILE *f, int nbrREC, int a) 
{
    block buffer; // Declare a buffer for storing block data
    buffer.address = 1; // Initialize the block address to 1

    // Calculate the number of blocks required to store all records
    // (Each block can hold up to 3 records)
    int taille = ceil((double)nbrREC / 3);

    // Move the file pointer to the position where the specified block starts
    fseek(f, a * sizeof(buffer), SEEK_SET);

    int k = 0; // Counter for the total number of records processed
    for (int i = 0; i < taille; i++) 
    {
        buffer.nbEnregistrement = 0; // Initialize the number of records in the block to 0
        int j = 0; // Counter for the number of records in the current block

        // Initialize the block by resetting all record fields
        for (int t = 0; t < 3; t++) 
        {
            buffer.tab[t].id = 0; // Reset the ID field
            strcpy(buffer.tab[t].data, ""); // Clear the data string
            buffer.tab[t].isDeleted = false; // Mark the record as not deleted
        }

        // Fill the current block with records (up to 3 or until all records are processed)
        while (j < 3 && k < nbrREC) 
        {
            printf("Enter record details:\n");
            printf("ID: ");
            scanf("%d", &buffer.tab[j].id); // Input the ID
            printf("Name: ");
            scanf(" %[^\n]", buffer.tab[j].data); // Input the data string
            buffer.tab[j].isDeleted = false; // Mark the record as not deleted
            buffer.nbEnregistrement++; // Increment the number of records in the block
            j++;
            k++;
        }

        // Check if the block is completely filled
        if (j == 3) 
        {
            buffer.isFree = false; // Mark the block as not free
        } 
        else 
        {
            buffer.isFree = true; // Mark the block as free
        }

        buffer.address++; // Increment the block address
        buffer.next = -1; // Set the next block address (indicating contiguous storage)

        // Write the block to the file
        fwrite(&buffer, sizeof(buffer), 1, f);
    }
}


// Function to create and initialize a new file with metadata
// Parameters:
// MD - pointer to the metadata file where file metadata will be stored
void Creation(FILE *MD) 
{
    int nbrREC; // Number of records to be stored in the new file
    char nomf[40]; // Name of the new file
    int count = 0; // Counter for the number of files created
    int currentMaxBlock; // Tracks the current maximum block address
    int A; // User choice for organization mode
    metaData Meta; // Metadata structure for the file

    // Prompt the user to enter the number of records (less than 50)
    printf("Enter the number of records (< 50): ");
    scanf("%d", &nbrREC);

    // Prompt the user to enter the name of the file
    printf("Enter the name of the file:\n");
    printf("File name: ");
    scanf(" %[^\n]", nomf);

    // Open the file in binary read/write mode
    FILE *f = fopen(nomf, "rb+");
    if (f == NULL) 
    {
        printf("Error: Could not open the file '%s'. Make sure the file exists and you have proper permissions.\n", nomf);
        return;
    }

    printf("File opened successfully.\n");
    count++; // Increment the file count

    // Initialize metadata
    Meta.sizeEnrgs = nbrREC; // Set the number of records
    Meta.fileId = count; // Assign a unique file ID

    // Calculate the number of blocks needed to store all records
    if (nbrREC % 3 == 0) 
    {
        Meta.sizeBlocks = nbrREC / 3;
    } 
    else 
    {
        Meta.sizeBlocks = (nbrREC / 3) + 1;
    }

    // Set the starting address and update the current maximum block address
    currentMaxBlock = 0;
    Meta.addressFirst = currentMaxBlock + 1; // Set the starting address
    currentMaxBlock += Meta.sizeBlocks; // Update the max block address

    // Copy the file name to the metadata structure
    strncpy(Meta.fileName, nomf, sizeof(Meta.fileName) - 1);
    Meta.fileName[sizeof(Meta.fileName) - 1] = '\0'; // Ensure null termination

    // Prompt the user to choose the organization mode
    printf("Enter '1' for contiguous sorted organization or '2' for chained unsorted organization:\n");
    scanf("%d", &A);

    // Set the organization mode based on user input
    switch (A) 
    {
        case 1:
            Meta.modeOrganizationGlobal = true; // Contiguous organization
            Meta.modeOrganizationIntern = true; // Sorted
            break;

        case 2:
            Meta.modeOrganizationGlobal = false; // Chained organization
            Meta.modeOrganizationIntern = false; // Unsorted
            break;

        default:
            printf("Invalid input. Please enter either '1' or '2'.\n");
            return;
    }

    // Write the metadata to the metadata file
    fwrite(&Meta, sizeof(Meta), 1, MD);
}


// Function to insert a new record into the file using the allocation table
// Parameters:
// f - file pointer to the data file
// TAB - allocation table containing the state of blocks
// sizeTAB - size of the allocation table
void insertion(FILE* f, BlockState TAB[], int sizeTAB) 
{
    rewind(f); // Move the file pointer to the beginning of the file
    block buffer; // Buffer to store block data
    enregistrement Rec; // New record to be inserted

    // Prompt the user to enter details of the new record
    printf("Enter the information for the new record:\n");
    printf("ID: ");
    scanf("%d", &Rec.id); // Input record ID
    printf("Data: ");
    scanf(" %[^\n]", Rec.data); // Input record data

    // Get the address of the last block from the allocation table
    int aDernierBloc = TAB[sizeTAB - 1].address;

    // Move the file pointer to the position of the last block
    if (fseek(f, aDernierBloc * sizeof(block), SEEK_SET) != 0) {
        printf("Error: Failed to move to the last block position.\n");
        return;
    }

    // Read the last block into the buffer
    if (fread(&buffer, sizeof(block), 1, f) != 1) {
        printf("Error: Failed to read the last block.\n");
        return;
    }

    // Check if there is space in the last block
    if (buffer.nbEnregistrement < 3) 
    {
        // Add the new record to the last block
        buffer.tab[buffer.nbEnregistrement].id = Rec.id;
        strcpy(buffer.tab[buffer.nbEnregistrement].data, Rec.data);
        buffer.tab[buffer.nbEnregistrement].isDeleted = false;
        buffer.nbEnregistrement++; // Increment the record count

        // If the block is now full, mark it as not free
        if (buffer.nbEnregistrement == 3) {
            buffer.isFree = false;
        }

        // Move the file pointer back to the position of the block
        if (fseek(f, -sizeof(block), SEEK_CUR) != 0) {
            printf("Error: Failed to move back to update the block.\n");
            return;
        }

        // Write the updated block back to the file
        fwrite(&buffer, sizeof(block), 1, f);
    } 
    else 
    {
        // If the last block is full, create a new block
        buffer.nbEnregistrement = 1;    // The new block will contain one record
        buffer.isFree = true;           // Initially, the new block is not full
        buffer.next = -1;               // No next block for now

        // Reset all records in the new block
        for (int i = 0; i < 3; i++) {
            buffer.tab[i].id = 0;
            strcpy(buffer.tab[i].data, "");
            buffer.tab[i].isDeleted = false;
        }

        // Add the new record to the first slot in the new block
        buffer.tab[0].id = Rec.id;
        strcpy(buffer.tab[0].data, Rec.data);
        buffer.tab[0].isDeleted = false;

        // Move the file pointer to the position for the new block
        if (fseek(f, (aDernierBloc + 1) * sizeof(block), SEEK_SET) != 0) {
            printf("Error: Failed to move to the position for the new block.\n");
            return;
        }

        // Write the new block to the file
        fwrite(&buffer, sizeof(block), 1, f);
        printf("A new block has been created and the record has been inserted.\n");
    }
}


// Function to search for a record in the file based on a given ID
// Parameters:
// f - pointer to the data file
// E - array to store the result [block number, record index]
// ID - the ID of the record to search for (provided by the user)
// TAB - allocation table representing the state of blocks
// sizeTAB - total number of blocks in the allocation table
void rechercheEnreg(FILE *f, int E[], int ID, BlockState TAB[], int sizeTAB) 
{
    block buffer; // Declare a buffer to hold block data
    rewind(f); // Reset the file pointer to the start of the file

    // Get the address of the first block and the last block
    int aPremierBloc = TAB[0].address;
    int aDernierBloc = TAB[sizeTAB - 1].address;

    // Move the file pointer to the first block's position
    fseek(f, aPremierBloc * sizeof(buffer), SEEK_SET);

    // Loop through all blocks in the allocation table
    for (int i = 0; i < sizeTAB; i++) 
    {
        // Read the current block into the buffer
        fread(&buffer, sizeof(buffer), 1, f);

        int j = 0; // Index for traversing records in the current block

        // Check each record in the block
        while (j < buffer.nbEnregistrement) 
        {
            if (buffer.tab[j].id == ID) // If the record ID matches the target ID
            {
                E[0] = i + 1; // Store the block number (1-based index)
                E[1] = j + 1; // Store the record index within the block (1-based index)
                printf("Block number: %d, Record index: %d\n", E[0], E[1]);
                return; // Exit the function after finding the record
            }
            j++; // Move to the next record in the block
        }
    }

    // If the loop completes without finding the record, print a message
    printf("The searched record does not exist!\n");
    return;
}


// Function to logically delete a record in the file based on its ID
// Parameters:
// f - pointer to the data file
// ID - the ID of the record to be logically deleted
// E - array containing the block and record position [block number, record index]
void suplogique(FILE *f, int ID, int E[]) 
{
    block buffer; // Declare a buffer to hold block data

    rewind(f); // Reset the file pointer to the start of the file

    // Move the file pointer to the position of the block that contains the target record
    // E[0] contains the block number (1-based index)
    fseek(f, E[0] * sizeof(buffer), SEEK_SET);

    fread(&buffer.tab[E[1]].id, sizeof(int), 1, f);

    // Mark the record as logically deleted by setting its isDeleted flag to true
    buffer.tab[E[1]].isDeleted = true;

    // Move the file pointer back to the position of the isDeleted flag
    fseek(f, -sizeof(int), SEEK_CUR);

    // Write the updated isDeleted flag back to the file
    fwrite(&buffer.tab[E[1]].isDeleted, sizeof(bool), 1, f);

    // Notify the user that the record has been logically deleted
    printf("Record with ID %d logically deleted successfully!\n", ID);
}

void loadFile(char fileName[], char mode) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;
    metaData newFile;
    int numRecords;
    char filename[40];
    
    // Read existing metadata
    if (fread(blockState, sizeof(BlockState), MaxBlocks, ms) != MaxBlocks ||
        fread(&filesMeta, sizeof(FilesMeta), 1, ms) != 1) {
        printf("Error reading file metadata\n");
        fclose(ms);
        return;
    }
    
    // Get file information
    printf("Enter filename: ");
    scanf("%39s", filename);  // Prevent buffer overflow
    printf("Enter number of records: ");
    scanf("%d", &numRecords);
    
    if (numRecords <= 0) {
        printf("Invalid number of records\n");
        fclose(ms);
        return;
    }

    // Calculate required blocks and check availability
    int requiredBlocks = (numRecords + 2) / 3;
    int freeBlockCount = 0;
    int firstFreeBlock = -1;
    
    for (int i = 0; i < MaxBlocks && freeBlockCount < requiredBlocks; i++) {
        if (blockState[i].free) {
            if (firstFreeBlock == -1) firstFreeBlock = i;
            freeBlockCount++;
        }
    }
    
    if (freeBlockCount < requiredBlocks) {
        printf("Not enough free blocks. Need %d, but only %d available\n", 
               requiredBlocks, freeBlockCount);
        fclose(ms);
        return;
    }

    // Initialize new file metadata
    strncpy(newFile.fileName, filename, sizeof(newFile.fileName) - 1);
    newFile.fileName[sizeof(newFile.fileName) - 1] = '\0';
    newFile.fileId = filesMeta.numberOf_files + 1;
    newFile.sizeBlocks = requiredBlocks;
    newFile.sizeEnrgs = numRecords;
    newFile.addressFirst = firstFreeBlock;
    newFile.modeOrganizationGlobal = (mode == 'c') ? false : true;
    newFile.modeOrganizationIntern = false;  // Default to unsorted

    // Initialize blocks
    block newBlock;
    int currentBlock = firstFreeBlock;
    int recordsLeft = numRecords;
    
    for (int i = 0; i < requiredBlocks; i++) {
        memset(&newBlock, 0, sizeof(block));
        newBlock.address = currentBlock;
        newBlock.nbEnregistrement = (recordsLeft >= 3) ? 3 : recordsLeft;
        newBlock.next = (mode == 'c' && i < requiredBlocks - 1) ? currentBlock + 1 : -1;
        newBlock.isFree = false;
        
        blockState[currentBlock].free = false;
        tableDallocation(fileName, 'a', currentBlock);
        
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + currentBlock * sizeof(block), SEEK_SET);
        if (fwrite(&newBlock, sizeof(block), 1, ms) != 1) {
            printf("Error writing block %d\n", currentBlock);
            fclose(ms);
            return;
        }
        
        recordsLeft -= newBlock.nbEnregistrement;
        currentBlock++;
    }
    
    // Update file metadata
    if (filesMeta.numberOf_files >= MaxBlocks) {
        printf("Maximum number of files reached\n");
        fclose(ms);
        return;
    }
    
    filesMeta.filesArray[filesMeta.numberOf_files] = newFile;
    filesMeta.numberOf_files++;
    
    fseek(ms, sizeof(BlockState), SEEK_SET);
    if (fwrite(&filesMeta, sizeof(FilesMeta), 1, ms) != 1) {
        printf("Error updating file metadata\n");
        fclose(ms);
        return;
    }
    
    printf("File loaded successfully\n");
    fclose(ms);
}

void searchRecordChained(char fileName[], int fileId, int recordId) {
    FILE *ms = fopen(fileName, "rb");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;
    block currentBlock;
    int blockNumber = 1, displacement = 1;
    
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);
    
    metaData *fileMeta = NULL;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            fileMeta = &filesMeta.filesArray[i];
            break;
        }
    }
    
    if (fileMeta == NULL) {
        printf("File not found!\n");
        fclose(ms);
        return;
    }
    
    int currentBlockAddress = fileMeta->addressFirst;
    
    while (currentBlockAddress != -1) {
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
        fread(&currentBlock, sizeof(block), 1, ms);
        
        for (int i = 0; i < currentBlock.nbEnregistrement; i++) {
            if (currentBlock.tab[i].id == recordId) {
                printf("Record found at Block: %d, Position: %d\n", blockNumber, displacement);
                printf("Data: %s\n", currentBlock.tab[i].data);
                fclose(ms);
                return;
            }
            displacement++;
        }
        currentBlockAddress = currentBlock.next;
        blockNumber++;
        displacement = 1;
    }
    
    printf("Record %d not found in file %d\n", recordId, fileId);
    fclose(ms);
}

void automaticFill(char fileName[], int fileId, int numberOfRecords) {

    if (numberOfRecords <= 0 || numberOfRecords > MaxBlocks * 3) {
    printf("Invalid number of records\n");
    return;
    }

    FILE *ms = fopen(fileName, "r+b");
    FilesMeta filesMeta;
    buffer buf;

    if (ms == NULL) {
        perror("Error opening file");
        return;
    }

    fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    metaData* currentFile = NULL;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            currentFile = &filesMeta.filesArray[i];
            break;
        }
    }

    if (currentFile == NULL) {
        printf("File not found\n");
        fclose(ms);
        return;
    }

    int recordsPerBlock = 3;
    int currentBlock = currentFile->addressFirst;
    int recordCount = 0;

    while (recordCount < numberOfRecords && 
           currentBlock < currentFile->addressFirst + currentFile->sizeBlocks) {

        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + 
              currentBlock * sizeof(buffer), SEEK_SET);
        fread(&buf, sizeof(buffer), 1, ms);

        while (buf.nbEnregistrement < recordsPerBlock && recordCount < numberOfRecords) {
            enregistrement newRecord;
            newRecord.id = recordCount + 1;
            snprintf(newRecord.data, sizeof(newRecord.data), "Data-%d", newRecord.id);
            newRecord.isDeleted = false;

            buf.tab[buf.nbEnregistrement] = newRecord;
            buf.nbEnregistrement++;
            recordCount++;
        }

        buf.isFree = false;
        buf.address = currentBlock;

        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + 
              currentBlock * sizeof(buffer), SEEK_SET);
        fwrite(&buf, sizeof(buffer), 1, ms);

        currentBlock++;
    }

    printf("Filled %d records in file %d\n", recordCount, fileId);
    fclose(ms);
}

// Function to defragment a file in the memory system
void defragmentFile(char fileName[], int fileId) {
    // Open the file in read-write binary mode
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks]; // Array to track block states
    FilesMeta filesMeta;             // Metadata for all files
    block currentBlock;              // Temporary block structure

    // Read block states and file metadata from the file
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    // Locate the metadata for the specified file using its ID
    metaData *fileMeta = NULL;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            fileMeta = &filesMeta.filesArray[i];
            break;
        }
    }

    // If file metadata is not found, print an error and exit
    if (fileMeta == NULL) {
        printf("File not found!\n");
        fclose(ms);
        return;
    }

    int currentBlockAddress = fileMeta->addressFirst; // Starting block of the file
    int totalValidRecords = 0;                        // Counter for valid records

    // First pass: Count the total number of valid (non-deleted) records
    while (currentBlockAddress != -1) {
        // Read the current block from the file
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
        fread(&currentBlock, sizeof(block), 1, ms);

        // Count valid records in the current block
        for (int i = 0; i < currentBlock.nbEnregistrement; i++) {
            if (!currentBlock.tab[i].isDeleted) {
                totalValidRecords++;
            }
        }
        currentBlockAddress = currentBlock.next; // Move to the next block
    }

    // Calculate the number of blocks required for valid records (3 records per block)
    int newRequiredBlocks = (totalValidRecords + 2) / 3;

    // Allocate memory to temporarily store all valid records
    enregistrement *validRecords = malloc(totalValidRecords * sizeof(enregistrement));
    if (validRecords == NULL) {
        printf("Memory allocation failed\n");
        fclose(ms);
        return;
    }

    // Second pass: Collect all valid records
    currentBlockAddress = fileMeta->addressFirst;
    int validIndex = 0;

    while (currentBlockAddress != -1) {
        // Read the current block
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
        fread(&currentBlock, sizeof(block), 1, ms);

        // Copy valid records to the temporary array
        for (int i = 0; i < currentBlock.nbEnregistrement; i++) {
            if (!currentBlock.tab[i].isDeleted) {
                validRecords[validIndex++] = currentBlock.tab[i];
            }
        }

        // Mark the current block as free
        blockState[currentBlockAddress].free = true;
        tableDallocation(fileName, 'f', currentBlockAddress);

        int nextBlock = currentBlock.next;
        currentBlockAddress = nextBlock; // Move to the next block
    }

    // Sort valid records by ID if the file is organized
    if (fileMeta->modeOrganizationIntern) {
        for (int i = 0; i < totalValidRecords - 1; i++) {
            for (int j = 0; j < totalValidRecords - i - 1; j++) {
                if (validRecords[j].id > validRecords[j + 1].id) {
                    enregistrement temp = validRecords[j];
                    validRecords[j] = validRecords[j + 1];
                    validRecords[j + 1] = temp;
                }
            }
        }
    }

    // Initialize variables for reorganizing blocks
    int firstNewBlock = -1;
    int currentNewBlock = -1;
    int recordsWritten = 0;

    // Redistribute valid records into new blocks
    for (int i = 0; i < newRequiredBlocks; i++) {
        // Find a free block
        int newBlockAddress = -1;
        for (int j = 0; j < MaxBlocks; j++) {
            if (blockState[j].free) {
                newBlockAddress = j;
                break;
            }
        }

        if (newBlockAddress == -1) {
            printf("Error: No free blocks available\n");
            free(validRecords);
            fclose(ms);
            return;
        }

        // Initialize a new block
        memset(&currentBlock, 0, sizeof(block));
        currentBlock.address = newBlockAddress;
        currentBlock.isFree = false;
        currentBlock.next = -1;

        // Fill the block with records
        int recordsInThisBlock = 0;
        while (recordsWritten < totalValidRecords && recordsInThisBlock < 3) {
            currentBlock.tab[recordsInThisBlock] = validRecords[recordsWritten];
            recordsWritten++;
            recordsInThisBlock++;
        }
        currentBlock.nbEnregistrement = recordsInThisBlock;

        // Update block linkage
        if (firstNewBlock == -1) {
            firstNewBlock = newBlockAddress;
        } else {
            // Update the previous block's next pointer
            block prevBlock;
            fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentNewBlock, SEEK_SET);
            fread(&prevBlock, sizeof(block), 1, ms);
            prevBlock.next = newBlockAddress;
            fseek(ms, -sizeof(block), SEEK_CUR);
            fwrite(&prevBlock, sizeof(block), 1, ms);
        }

        // Write the new block to the file
        blockState[newBlockAddress].free = false;
        tableDallocation(fileName, 'a', newBlockAddress);
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * newBlockAddress, SEEK_SET);
        fwrite(&currentBlock, sizeof(block), 1, ms);

        currentNewBlock = newBlockAddress;
    }

    // Update file metadata with new block information
    fileMeta->addressFirst = firstNewBlock;
    fileMeta->sizeBlocks = newRequiredBlocks;
    fileMeta->sizeEnrgs = totalValidRecords;

    // Write the updated metadata back to the file
    fseek(ms, sizeof(BlockState), SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    free(validRecords); // Free allocated memory
    fclose(ms);         // Close the file
    printf("File defragmentation completed successfully.\n");
}

// Function to rename a file in the system
void renameFile(char fileName[], int fileId, char newName[]) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;

    // Read block states and file metadata
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    // Locate the file using its ID
    bool found = false;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            // Update the file name
            strncpy(filesMeta.filesArray[i].fileName, newName, sizeof(filesMeta.filesArray[i].fileName) - 1);
            filesMeta.filesArray[i].fileName[sizeof(filesMeta.filesArray[i].fileName) - 1] = '\0';
            found = true;
            break;
        }
    }

    if (!found) {
        printf("File with ID %d not found\n", fileId);
        fclose(ms);
        return;
    }

    // Write updated metadata back to the file
    fseek(ms, sizeof(BlockState), SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fclose(ms);
    printf("File renamed successfully\n");
}


// Function to delete a file and free its blocks
void deleteFile(char fileName[], int fileId) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;

    // Read block states and file metadata
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    // Locate the file using its ID
    int fileIndex = -1;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            fileIndex = i;
            break;
        }
    }

    if (fileIndex == -1) {
        printf("File with ID %d not found\n", fileId);
        fclose(ms);
        return;
    }

    // Free all blocks used by the file
    int currentBlock = filesMeta.filesArray[fileIndex].addressFirst;
    block currentBlockData;

    while (currentBlock != -1) {
        // Read the current block to get the next block address
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlock, SEEK_SET);
        fread(&currentBlockData, sizeof(block), 1, ms);

        // Mark the block as free
        blockState[currentBlock].free = true;
        tableDallocation(fileName, 'f', currentBlock);

        int nextBlock = currentBlockData.next;

        // Clear the block data
        memset(&currentBlockData, 0, sizeof(block));
        currentBlockData.address = currentBlock;
        currentBlockData.isFree = true;
        currentBlockData.next = -1;

        // Write the cleared block back to the file
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlock, SEEK_SET);
        fwrite(&currentBlockData, sizeof(block), 1, ms);

        currentBlock = nextBlock;
    }

    // Remove the file from metadata by shifting remaining files
    for (int i = fileIndex; i < filesMeta.numberOf_files - 1; i++) {
        filesMeta.filesArray[i] = filesMeta.filesArray[i + 1];
    }
    filesMeta.numberOf_files--;

    // Write updated metadata back to the file
    fseek(ms, sizeof(BlockState), SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fclose(ms);
    printf("File deleted successfully\n");
}

int main() {
    char fileName[40] = "disk.bin";  // Default disk file name
    int choice;
    bool running = true;

    while (running) {
        printf("\n=== SECONDARY MEMORY MANAGEMENT SYSTEM ===\n");
        printf("1.  Initialize Secondary Memory\n");
        printf("2.  Create and Load File\n");
        printf("3.  Display Secondary Memory State (Graphical)\n");
        printf("4.  Display File Metadata\n");
        printf("5.  Search Record by ID\n");
        printf("6.  Insert New Record\n");
        printf("7.  Delete Record\n");
        printf("8.  Defragment File\n");
        printf("9.  Delete File\n");
        printf("10. Rename File\n");
        printf("11. Compact Secondary Memory\n");
        printf("12. Clear Secondary Memory\n");
        printf("13. Exit\n");
        printf("Choose an option (1-13): ");
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: {
                initialisationDisque(fileName);
                printf("Secondary memory initialized successfully.\n");
                break;
            }
            
            case 2: {
                char mode;
                printf("Choose organization mode (c for chained, t for contiguous): ");
                scanf(" %c", &mode);
                loadFile(fileName, mode);
                break;
            }
            
            case 3: {
                FILE *ms = fopen(fileName, "rb");
                if (ms == NULL) {
                    printf("Error opening disk file\n");
                    break;
                }
                
                BlockState *blockStates = malloc(MaxBlocks * sizeof(BlockState));
                if (blockStates == NULL) {
                    printf("Memory allocation failed\n");
                    fclose(ms);
                    break;
                }
                
                FilesMeta filesMeta;
                block currentBlock;
                
                fread(blockStates, sizeof(BlockState), MaxBlocks, ms);
                fread(&filesMeta, sizeof(FilesMeta), 1, ms);


                printf("\nSecondary Memory State:\n");
                for (int i = 0; i < MaxBlocks; i++) {
                    if (blockStates[i].free) {
                        printf("\033[0;32m[FREE]\033[0m ");  // Green for free blocks
                    } else {
                        printf("\033[0;31m[USED]\033[0m ");  // Red for used blocks
                        
                        // Read block to get number of records
                        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + i * sizeof(block), SEEK_SET);
                        fread(&currentBlock, sizeof(block), 1, ms);
                        
                        // Find which file this block belongs to
                        for (int j = 0; j < filesMeta.numberOf_files; j++) {
                            if (i >= filesMeta.filesArray[j].addressFirst && 
                                i < filesMeta.filesArray[j].addressFirst + filesMeta.filesArray[j].sizeBlocks) {
                                printf("'%s' (%d records) ", 
                                       filesMeta.filesArray[j].fileName,
                                       currentBlock.nbEnregistrement);
                                break;
                            }
                        }
                    }
                    if ((i + 1) % 5 == 0) printf("\n");  // New line every 5 blocks
                }
                fclose(ms);
                free(blockStates);
                fclose(ms);
                break;
            }
            
            case 4: {
                FILE *ms = fopen(fileName, "rb");
                if (ms == NULL) {
                    printf("Error opening disk file\n");
                    break;
                }
                
                FilesMeta filesMeta;
                fseek(ms, sizeof(BlockState) * MaxBlocks, SEEK_SET);
                fread(&filesMeta, sizeof(FilesMeta), 1, ms);
                
                printf("\nFile Metadata:\n");
                printf("%-20s %-8s %-12s %-12s %-15s %-15s %-15s\n",
                       "Filename", "FileID", "Total Blocks", "Total Records", "First Block", "Global Org", "Internal Org");
                printf("--------------------------------------------------------------------------------\n");
                
                for (int i = 0; i < filesMeta.numberOf_files; i++) {
                    printf("%-20s %-8d %-12d %-12d %-15d %-15s %-15s\n",
                           filesMeta.filesArray[i].fileName,
                           filesMeta.filesArray[i].fileId,
                           filesMeta.filesArray[i].sizeBlocks,
                           filesMeta.filesArray[i].sizeEnrgs,
                           filesMeta.filesArray[i].addressFirst,
                           filesMeta.filesArray[i].modeOrganizationGlobal ? "Contiguous" : "Chained",
                           filesMeta.filesArray[i].modeOrganizationIntern ? "Sorted" : "Unsorted");
                }
                fclose(ms);
                break;
            }
            
            case 5: {
                int fileId, recordId;
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                printf("Enter Record ID to search: ");
                scanf("%d", &recordId);
                searchRecordChained(fileName, fileId, recordId);
                break;
            }
            
            case 6: {
            int fileId;
            enregistrement newRecord;


            printf("Enter File ID: ");
            if (scanf("%d", &fileId) != 1) {
                printf("Invalid File ID. Please enter a numeric value.\n");
                break;
            }
            getchar(); // Clear buffer

            // Get Record ID
            printf("Enter Record ID: ");
            if (scanf("%d", &newRecord.id) != 1) {
                printf("Invalid Record ID. Please enter a numeric value.\n");
                break;
            }
            getchar(); // Clear buffer

            // Get Record Data
            printf("Enter Record Data: ");
            if (scanf(" %[^\n]", newRecord.data) != 1) {
                printf("Invalid Record Data. Please try again.\n");
                break;
            }
            newRecord.isDeleted = false;

            // Call the insertion function
            insertRecordChained(fileName, fileId, newRecord);
            break;
        }

            
            case 7: {
                int fileId, recordId;
                char deleteType;
                printf("Enter File ID: ");
                scanf("%d", &fileId);
                printf("Enter Record ID to delete: ");
                scanf("%d", &recordId);
                printf("Choose delete type (p for physical, l for logical): ");
                scanf(" %c", &deleteType);
                
                if (deleteType == 'p') {
                    physicalDelete(fileName, fileId, recordId);
                } else {
                    // Call logical delete function (not implemented in the provided code)
                    printf("Logical delete not implemented\n");
                }
                break;
            }
            
            case 8: {
                int fileId;
                printf("Enter File ID to defragment: ");
                scanf("%d", &fileId);
                // Call defragment function (not implemented in the provided code)
                printf("File defragmentation not implemented\n");
                break;
            }
            
            case 9: {
                int fileId;
                printf("Enter File ID to delete: ");
                scanf("%d", &fileId);
                // Call delete file function (not implemented in the provided code)
                printf("File deletion not implemented\n");
                break;
            }
            
            case 10: {
                int fileId;
                char newName[40];
                printf("Enter File ID to rename: ");
                scanf("%d", &fileId);
                printf("Enter new filename: ");
                scanf(" %[^\n]", newName);
                // Call rename function (not implemented in the provided code)
                printf("File rename not implemented\n");
                break;
            }
            
            case 11: {
                compactage(fileName);
                printf("Secondary memory compacted successfully.\n");
                break;
            }
            
            case 12: {
                vider(fileName);
                printf("Secondary memory cleared successfully.\n");
                break;
            }
            
            case 13: {
                running = false;
                printf("Exiting program...\n");
                break;
            }
            
            default: {
                printf("Invalid option. Please choose a number between 1 and 13.\n");
                break;
            }
        }
    }
    
    return 0;
}