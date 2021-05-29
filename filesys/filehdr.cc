// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------



void
IndirectSector::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
} 

void
IndirectSector::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}
int
IndirectSector::getSector(int index)
{
   return dataSectors[index];
}

bool
IndirectSector::Allocate(BitMap *freeMap)
{ 
    if (freeMap->NumClear() < NumInIndirect) return FALSE;
    for (int i = 0; i < NumInIndirect; i++)
        dataSectors[i] = freeMap->Find();
    return TRUE;
}

void
IndirectSector::Deallocate(BitMap *freeMap)
{ 
    for (int i = 0; i < NumInIndirect; i++) {
	    ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
    }
}


bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    if (fileSize > MaxFileSize) return FALSE;

    numBytes = fileSize;
    int numTotalSectors = divRoundUp(fileSize, SectorSize);
    int numDirectSectors = min(numTotalSectors, NumDirect);


    int numIndirectSectors = divRoundUp(numTotalSectors - numDirectSectors, NumInIndirect);
    printf("%d", numIndirectSectors);

    if (freeMap->NumClear() < numDirectSectors + numIndirectSectors + numIndirectSectors * NumInIndirect)
	    return FALSE;		// not enough space
    
    for (int i = 0; i < numDirectSectors; i++)
	    dataSectors[i] = freeMap->Find();

    for (int i = 0; i < numIndirectSectors; i++){
        indirectDataSectors[i] = freeMap->Find();
        IndirectSector* inds = new IndirectSector();

        inds->Allocate(freeMap);
        inds->WriteBack(indirectDataSectors[i]);
        delete inds;
    }
    return TRUE;
}

bool
FileHeader::AllocateMore(BitMap *freeMap, int Size)
{
    int numTotalSectors = divRoundUp(numBytes, SectorSize);
    int numDirectSectors = min(numTotalSectors, NumDirect);
    int numIndirectSectors = divRoundUp(numTotalSectors - numDirectSectors, NumInIndirect);
    

    if (numBytes + Size > MaxFileSize) return FALSE;

    int new_numBytes = numBytes + Size;
    int new_numTotalSectors = divRoundUp(new_numBytes, SectorSize);
    int new_numDirectSectors = min(new_numTotalSectors, NumDirect);
    int new_numIndirectSectors = divRoundUp(new_numTotalSectors - new_numDirectSectors, NumInIndirect);

    if (freeMap->NumClear() 
        < new_numDirectSectors - numDirectSectors 
        + new_numIndirectSectors - numIndirectSectors
        + (new_numIndirectSectors - numIndirectSectors) * NumInIndirect) 
            return FALSE;		// not enough space

    for (int i = numDirectSectors; i < new_numDirectSectors; i++)
        dataSectors[i] = freeMap->Find();
    
    for (int i = numIndirectSectors; i < new_numIndirectSectors; i++){
        indirectDataSectors[i] = freeMap->Find();
        IndirectSector* inds = new IndirectSector();
        bool ret = inds->Allocate(freeMap);
        inds->WriteBack(indirectDataSectors[i]);
        delete inds;
    }
    numBytes += Size;
    return TRUE;
}


//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{ 
    int numTotalSectors = divRoundUp(numBytes, SectorSize);
    int numDirectSectors = min(numTotalSectors, NumDirect);
    int numIndirectSectors = divRoundUp(numTotalSectors - numDirectSectors, NumInIndirect);

    for (int i = 0; i < numDirectSectors; i++) {
	    ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) dataSectors[i]);
    }
    for (int i = 0; i < numIndirectSectors; i++){
        IndirectSector* inds = new IndirectSector();
        
        inds->FetchFrom(indirectDataSectors[i]);
        inds->Deallocate(freeMap);
        
        ASSERT(freeMap->Test((int) indirectDataSectors[i]));  // ought to be marked!
	    freeMap->Clear((int) indirectDataSectors[i]);

        delete inds;
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int sectorIndex = offset / SectorSize;
    if (sectorIndex < NumDirect) return(dataSectors[sectorIndex]);
    int indirectSectorIndex = (sectorIndex - NumDirect) / NumInIndirect;   
    IndirectSector* indirect_sector = new IndirectSector();
    indirect_sector->FetchFrom(indirectDataSectors[indirectSectorIndex]);

    int ret = indirect_sector->getSector((sectorIndex - NumDirect) % NumInIndirect);
    delete indirect_sector;
    return ret;
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int numTotalSectors = divRoundUp(numBytes, SectorSize);
    int numDirectSectors = min(numTotalSectors, NumDirect);
    int numIndirectSectors = divRoundUp(numTotalSectors - numDirectSectors, NumInIndirect);
    
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numDirectSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numDirectSectors; i++) {
	synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

void FileHeader::SetType(int type){
    fileType = type;
}