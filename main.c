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


void saisir(FILE *f,int nbrREC,int a) //a = numero de block , nbrREC = (nbr total des record qu'en veut ajoute ) (tout lire depuis fichier metadone)
{
   block buffer;
   buffer.address = 1 ;

    int taille = ceil((double)nbrREC / 3); //on peut lire la taille depuis le fichier metadonne = size blocks

    fseek(f, a*sizeof(buffer), SEEK_SET);

    int k = 0;
    for(int i=0; i<taille; i++)
    {
        buffer.nbEnregistrement = 0;
        int j = 0;

        //initialize the block
        for (int t = 0; t < 3; t++) {
        buffer.tab[t].id = 0;
        strcpy(buffer.tab[t].data, "");
        buffer.tab[t].isDeleted = false;
 }

        while(j<3 && k<nbrREC )
        {
            printf("donne l'identite :\n" );
            printf("ID : ");
            scanf("%d", &buffer.tab[j].id);
            printf("donne data string :\n");
            printf("Nom : ");
            scanf(" %[^\n]", buffer.tab[j].data);
            buffer.tab[j].isDeleted = false;
            buffer.nbEnregistrement++; //incrementer le nombre d'enregistremant
            j++;
            k++;
        }
      //check if we completed all the place in one block
            if(j==3){
              buffer.isFree = false;
            }else{
              buffer.isFree = true;
            }

         buffer.address++ ; //incrementer le numero du block
         buffer.next = -1; // contigue

        fwrite(&buffer, sizeof(buffer), 1, f);
    }
}


void Creation(FILE *MD) //teste si MD != null dans le menu principal si vrai apeler cette fonction
{
 int nbrREC;
 char nomf [40];
 int count = 0;
 int currentMaxBlock  ;
    int A ;
    metaData Meta ;

 printf("donne le nombre d' enregistremant < 50 : ");
 scanf("%d",&nbrREC);

 printf("donne le nom du fichier :/n");
 printf("nom fichier = ");
 scanf(" %[^\n]", nomf);

 FILE *f =fopen(nomf,"rb+"); //ouverture du fichier
 if(f == NULL){
   printf("Error: Could not open the file '%s'. Make sure the file exists and you have proper permissions.\n", nomf);
            return;
 }

  printf("file opened with success. \n");
  count ++; //incrementer le nombre de fichier

  Meta.sizeEnrgs = nbrREC;
        Meta.fileId = count; // unique file ID

  if(nbrREC % 3 == 0){ // calculer le nombre du block neccesaire pour ce fichier
       Meta.sizeBlocks = nbrREC / 3;
  }
  else{
    Meta.sizeBlocks = (nbrREC / 3) +1;
  }

  // Starting from the first block
  currentMaxBlock = 0;

  Meta.addressFirst = currentMaxBlock + 1; // Set starting address
        currentMaxBlock += Meta.sizeBlocks;     // Update max block address



        // Copy file name to metadata
         strncpy(Meta.fileName, nomf, sizeof(Meta.fileName) - 1);
         Meta.fileName[sizeof(Meta.fileName) - 1] = '\0'; // Ensure null termination


   printf("write number '1' if you want organisation contigu trie else write number '2' if you want organisation chaine non trie. \n");
   scanf("%d ", &A);

 switch (A){
        case 1:
            Meta.modeOrganizationGlobal = true ; // contigue
            Meta.modeOrganizationIntern = true ; // trie
            break;

        case 2:
            Meta.modeOrganizationGlobal = false ; // chaine
            Meta.modeOrganizationIntern = false ; // non trie
            break;

        default:
            printf("Invalid number \n");
            return;
    }

    fwrite(&Meta, sizeof(Meta), 1,MD); // Write metadata to the metadata file
}


void insertion(FILE* f,BlockState TAB[],int sizeTAB) // en utulise le tableau d'allocation
{
    rewind(f);
    block buffer;
    enregistrement Rec;

    printf("Donne les information du nouveau enregistrement :\n");
    printf("ID : ");
    scanf("%d", &Rec.id);
    printf("data : ");
    scanf(" %[^\n]", Rec.data);

    int aDernierBloc = TAB[sizeTAB-1].address ; // trouve l'index du dernier block

    // Move to the last block
    if (fseek(f, aDernierBloc * sizeof(block), SEEK_SET) != 0) {
        printf("Error: Failed to move to the last block position.\n");
        return;
    }

    // Read the last block
    if (fread(&buffer, sizeof(block), 1, f) != 1) {
        printf("Error: Failed to read the last block.\n");
        return;
    }

    if(buffer.nbEnregistrement < 3) // si il ya de place dans le dernier block
    {
        buffer.tab[buffer.nbEnregistrement].id = Rec.id;
        strcpy(buffer.tab[buffer.nbEnregistrement].data, Rec.data);
       buffer.tab[buffer.nbEnregistrement].isDeleted = false;
        buffer.nbEnregistrement++;

  if(buffer.nbEnregistrement == 3)
      {buffer.isFree = false ; //block is full
  }

      // Move back to update the block
        if (fseek(f, -sizeof(block), SEEK_CUR) != 0) {
            printf("Error: Failed to move back to update the block.\n");
            return;
        }

        // Write the updated block
        fwrite(&buffer, sizeof(block), 1, f);

    }
    else                               // si ya pas il faut ajoute dans un autre block
    {
        // Reinitialize buffer for the new block
        buffer.nbEnregistrement = 1;    // The new block has one record
        buffer.isFree = true;           // The new block is initially not full
        buffer.next = -1;

        for (int i = 0; i < 3; i++) {   // Reset all slots in the block
        buffer.tab[i].id = 0;
        strcpy(buffer.tab[i].data, "");
        buffer.tab[i].isDeleted = false;
        }

        // Insert the new record into the first case in the vector
        buffer.tab[0].id = Rec.id;
        strcpy(buffer.tab[0].data, Rec.data);
        buffer.tab[0].isDeleted = false;


        // Write the new block to the file
        if (fseek(f, (aDernierBloc + 1) * sizeof(block), SEEK_SET) != 0) {
            printf("Error: Failed to move to the position for the new block.\n");
            return;
        }

        fwrite(&buffer, sizeof(block), 1, f);
        printf("A new block has been created and the record has been inserted.\n");
    }

}


void rechercheEnreg(FILE *f, int E[], int ID,BlockState TAB [],int sizeTAB) //ID donne par user
{
    block buffer;
    rewind(f);

    int aPremierBloc = TAB[0].address;
    int aDernierBloc = TAB[sizeTAB-1].address ;

    fseek(f, aPremierBloc*sizeof(buffer), SEEK_SET); //aller au premier block

    for(int i=0; i<sizeTAB; i++) //parcourir jusq au dernier block
    {
        fread(&buffer, sizeof(buffer), 1, f);
        int j = 0;
        while(j<buffer.nbEnregistrement)
        {
            if(buffer.tab[j].id == ID)
            {
                E[0] = i+1;
                E[1] = j+1;
                printf("numero du Bloc: %d, deplacement: %d\n", E[0], E[1]);
                return;
            }
            j++;
        }
    }
    printf("l’enregistrement recherché n’existe pas. !! ");
    return;
}


void suplogique(FILE *f,int ID,int E[]) // on utilise le meme id de la recherche
{   block buffer;

 rewind(f);

 fseek(f,E[0]*sizeof(buffer),SEEK_SET); //aller a la position du block qui a le ID

  fread(&buffer.tab[E[1]].id, sizeof(int), 1, f); //E[1] = la position de l'eregistremant

  buffer.tab[E[1]].isDeleted = true; // record logicement suprime
  fseek(f, -sizeof(int), SEEK_CUR);
  fwrite(&buffer.tab[E[1]].isDeleted, sizeof(bool), 1, f);


    printf("Etudiant %d supprimé logiquement avec succés !\n", ID);

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
        
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlock, SEEK_SET);
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

void defragmentFile(char fileName[], int fileId) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;
    block currentBlock;

    // Read block states and file metadata
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    // Find the file metadata
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
    int totalValidRecords = 0;

    // First pass: Count valid records
    while (currentBlockAddress != -1) {
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
        fread(&currentBlock, sizeof(block), 1, ms);

        for (int i = 0; i < currentBlock.nbEnregistrement; i++) {
            if (!currentBlock.tab[i].isDeleted) {
                totalValidRecords++;
            }
        }
        currentBlockAddress = currentBlock.next;
    }

    // Calculate new required blocks
    int newRequiredBlocks = (totalValidRecords + 2) / 3;
    
    // Temporary array to store valid records
    enregistrement *validRecords = malloc(totalValidRecords * sizeof(enregistrement));
    if (validRecords == NULL) {
        printf("Memory allocation failed\n");
        fclose(ms);
        return;
    }

    // Second pass: Collect valid records
    currentBlockAddress = fileMeta->addressFirst;
    int validIndex = 0;
    
    while (currentBlockAddress != -1) {
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlockAddress, SEEK_SET);
        fread(&currentBlock, sizeof(block), 1, ms);

        for (int i = 0; i < currentBlock.nbEnregistrement; i++) {
            if (!currentBlock.tab[i].isDeleted) {
                validRecords[validIndex++] = currentBlock.tab[i];
            }
        }
        
        // Mark current block as free
        blockState[currentBlockAddress].free = true;
        tableDallocation(fileName, 'f', currentBlockAddress);
        
        int nextBlock = currentBlock.next;
        currentBlockAddress = nextBlock;
    }

    // Sort valid records if file is organized
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

    // Find new blocks for reorganized file
    int firstNewBlock = -1;
    int currentNewBlock = -1;
    int recordsWritten = 0;

    // Redistribute records into new blocks
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

        // Initialize new block
        memset(&currentBlock, 0, sizeof(block));
        currentBlock.address = newBlockAddress;
        currentBlock.isFree = false;
        currentBlock.next = -1;

        // Fill block with records
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
            // Update previous block's next pointer
            block prevBlock;
            fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentNewBlock, SEEK_SET);
            fread(&prevBlock, sizeof(block), 1, ms);
            prevBlock.next = newBlockAddress;
            fseek(ms, -sizeof(block), SEEK_CUR);
            fwrite(&prevBlock, sizeof(block), 1, ms);
        }

        // Write new block
        blockState[newBlockAddress].free = false;
        tableDallocation(fileName, 'a', newBlockAddress);
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * newBlockAddress, SEEK_SET);
        fwrite(&currentBlock, sizeof(block), 1, ms);

        currentNewBlock = newBlockAddress;
    }

    // Update file metadata
    fileMeta->addressFirst = firstNewBlock;
    fileMeta->sizeBlocks = newRequiredBlocks;
    fileMeta->sizeEnrgs = totalValidRecords;

    // Write updated metadata
    fseek(ms, sizeof(BlockState), SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    free(validRecords);
    fclose(ms);
    printf("File defragmentation completed successfully.\n");
}

void renameFile(char fileName[], int fileId, char newName[]) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;

    // Read metadata
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    // Find the file
    bool found = false;
    for (int i = 0; i < filesMeta.numberOf_files; i++) {
        if (filesMeta.filesArray[i].fileId == fileId) {
            // Update filename
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

    // Write updated metadata
    fseek(ms, sizeof(BlockState), SEEK_SET);
    fwrite(&filesMeta, sizeof(FilesMeta), 1, ms);

    fclose(ms);
    printf("File renamed successfully\n");
}

void deleteFile(char fileName[], int fileId) {
    FILE *ms = fopen(fileName, "rb+");
    if (ms == NULL) {
        printf("Error opening file\n");
        return;
    }

    BlockState blockState[MaxBlocks];
    FilesMeta filesMeta;

    // Read metadata
    fread(blockState, sizeof(BlockState), MaxBlocks, ms);
    fread(&filesMeta, sizeof(FilesMeta), 1, ms);

    // Find the file
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
        // Read current block to get next block address
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlock, SEEK_SET);
        fread(&currentBlockData, sizeof(block), 1, ms);
        
        // Mark block as free
        blockState[currentBlock].free = true;
        tableDallocation(fileName, 'f', currentBlock);

        int nextBlock = currentBlockData.next;
        
        // Clear block data
        memset(&currentBlockData, 0, sizeof(block));
        currentBlockData.address = currentBlock;
        currentBlockData.isFree = true;
        currentBlockData.next = -1;
        
        // Write cleared block back
        fseek(ms, sizeof(BlockState) * MaxBlocks + sizeof(FilesMeta) + sizeof(block) * currentBlock, SEEK_SET);
        fwrite(&currentBlockData, sizeof(block), 1, ms);

        currentBlock = nextBlock;
    }

    // Remove file from metadata by shifting remaining files
    for (int i = fileIndex; i < filesMeta.numberOf_files - 1; i++) {
        filesMeta.filesArray[i] = filesMeta.filesArray[i + 1];
    }
    filesMeta.numberOf_files--;

    // Write updated metadata
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
                
                BlockState blockStates[MaxBlocks];
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