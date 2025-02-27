# Secondary Memory Management Simulator

This project implements a **Secondary Memory Management System** in C. The program simulates operations for managing files and records stored on secondary memory.

---

## Features

1. **Disk Initialization**: Reset and initialize the entire disk.
2. **File Management**:
   - Create files with options for contiguous or chained storage.
   - Rename or delete files.
3. **Record Management**:
   - Insert new records.
   - Search for records by their ID.
   - Delete records logically (mark as deleted) or physically (permanently remove).
   - Defragment files to remove logically deleted records.
4. **Disk Optimization**:
   - Compact the disk to eliminate fragmentation.
   - Display the disk state and file metadata.
5. **Disk Clearing**: Clear the entire disk.

---

## Menu Options

After running the program, you will be presented with a menu:

1. **Initialize Disk**: Clears all blocks and resets disk metadata.
2. **Create and Load File**: Create a new file with specified properties (contiguous or chained storage, sorted or unsorted).
3. **Insert Record**: Add a new record to an existing file.
4. **Search Record**: Search for a record by its ID in a specific file.
5. **Delete Record (Logical)**: Mark a record as deleted without physically removing it.
6. **Delete Record (Physical)**: Remove a record permanently and adjust storage.
7. **Defragment File**: Remove logically deleted records and compress the file.
8. **Rename File**: Rename an existing file.
9. **Delete File**: Remove a file and release its allocated blocks.
10. **Compact Disk**: Consolidate free blocks to reduce fragmentation.
11. **Clear Disk**: Reset all files and blocks.
12. **Display Disk State**: Visual representation of the disk (used/free blocks).
13. **Display File Metadata**: Show information about all created files.
14. **Exit**: Exit the program.

---

## Example Usage

### Creating and Managing Files

1. **Create a File**:

   - Choose option `2` from the menu.
   - Enter the file name, number of records, and the storage type (0 for contiguous, 1 for chained).

2. **Insert a Record**:

   - Choose option `3` and specify the file ID.
   - Enter the record's ID and data.

3. **Search for a Record**:

   - Choose option `4`, specify the file ID and the record ID to search.

4. **Delete a Record**:

   - Choose option `5` (logical delete) or `6` (physical delete).
   - Provide the file ID and record ID.

5. **Compact and Defragment**:

   - Choose option `7` to defragment a specific file.
   - Choose option `10` to compact the entire disk.

### Displaying Information

- **Display Disk State**: Use option `12` to visualize the disk (free/used blocks).
- **Display Metadata**: Use option `13` to see file properties.

---

## Notes

- Ensure enough free blocks are available before creating a file.
- Logical deletion marks records as deleted, but physical deletion frees up space immediately.
- Compacting the disk rearranges all blocks and can affect file ordering.

---

## Example Menu Flow

```text
=== SECONDARY MEMORY MANAGEMENT SYSTEM ===
1. Initialize Disk
2. Create and Load File
3. Insert Record
...
14. Exit
Choose an option (1-14): 2
Enter file name: MyFile
Enter number of records: 5
Choose organization (0 for contiguous, 1 for chained): 0
Choose sorting (0 for unsorted, 1 for sorted): 0
File created successfully with ID: 0

=== SECONDARY MEMORY MANAGEMENT SYSTEM ===
1. Initialize Disk
2. Create and Load File
3. Insert Record
...
14. Exit
Choose an option (1-14): 3
Enter File ID: 0
Enter record ID: 101
Enter record data: HelloWorld
Record inserted successfully.
```

---

## Limitations

- Maximum of 30 blocks and 3 records per block.
- Disk compaction and defragmentation may be time-consuming for large files.

