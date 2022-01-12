/*

������ ��������� MZ-������.

��������� ������� ��������, ������ ���� �������������     30.04.2020

*/


#include <windows.h>
#include <stdio.h>
#include <malloc.h>

#include "mz.h"
#include "system.h"


//
// ��������� ���� � ������ � ��������� ��������� MzHeaders.
//
BOOL LoadMZFile(char *filename, MZHeaders *mz) {

    mz->filename = filename;

    // ��������� ���� (�������� �������� ����������)
    mz->fd = CreateFileA(
        filename,                     // ��� �����
        GENERIC_READ | GENERIC_WRITE, // ����� �������
        0,
        NULL,
        OPEN_EXISTING,                // ����������� ���� ������ ������������
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (mz->fd == INVALID_HANDLE_VALUE) {
        PrintLastErrorMessage();
        return FALSE;
    }

    mz->filesize = GetFileSize(mz->fd, NULL);

    // ������� �������� ����� � ������
    mz->mapd = CreateFileMappingA(mz->fd, NULL, PAGE_READWRITE, 0, mz->filesize, NULL);
    if (mz->mapd == NULL) {
        CloseHandle(mz->fd);
        printf("Error create file map\n");
        PrintLastErrorMessage();
        return FALSE;
    }

    // ���������� �������� � ������
    mz->mem = (PBYTE) MapViewOfFile(mz->mapd, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
    if (mz->mem == NULL) {
        CloseHandle(mz->fd);
        CloseHandle(mz->mapd);
        printf("Error mapping file\n");
        PrintLastErrorMessage();
        return FALSE;
    }

    // ��������� �� ��������� mz
    mz->doshead = (PDOS_HEADER) mz->mem;

    // �������� ���������
    if (mz->doshead->e_magic != MZ_SIGNATURE) {
        UnmapViewOfFile(mz->mem);
        CloseHandle(mz->fd);
        CloseHandle(mz->mapd);
        printf("Error DOS signature\n");
        return FALSE;
    }

    // ��������
    mz->data = mz->mem + (mz->doshead->e_cparhdr * PARAGRAPH_SIZE);
    mz->stack = mz->data + (mz->doshead->e_ss * PARAGRAPH_SIZE);
    mz->code = mz->data + (mz->doshead->e_cs * PARAGRAPH_SIZE);
    if (mz->doshead->e_ss < mz->doshead->e_cs) {
        mz->data_size = mz->stack - mz->data;
        mz->stack_size = mz->code - mz->stack;
        mz->code_size = (mz->mem + mz->filesize) - mz->code;
    }
    else {
        mz->data_size = mz->code - mz->data;
        mz->code_size = mz->stack - mz->code;
        mz->stack_size = (mz->mem + mz->filesize) - mz->stack;
    }

    // ������� �������
    mz->relocs = (PRELOC_TABLE_ENTRY) (mz->mem + mz->doshead->e_lfarlc);
    mz->relocs_size = mz->doshead->e_crlc;

    return TRUE;
}


//
// ������� ���������� �������.
//
void ApplyRelocs(MZHeaders *mz, WORD imageBase) {

    for (int i = 0; i < mz->relocs_size; i++)
    {
        PBYTE tmp = mz->data + mz->relocs[i].segment * 16 * sizeof(BYTE) + mz->relocs[i].offset;
        *(PWORD)tmp += imageBase;
    }

    return;
}


//
// ������� ������ �������.
//
void RemoveRelocs(MZHeaders *mz, WORD imageBase) {

    for (int i = 0; i < mz->relocs_size; i++)
    {
        PBYTE tmp = mz->data + mz->relocs[i].segment * 16 * sizeof(BYTE) + mz->relocs[i].offset;
        *(PWORD)tmp -= imageBase;
    }

    return;
}


//
// ��������� MZ-���� (����������� �������).
//
void UnloadMZFile(MZHeaders *mz) {

    UnmapViewOfFile(mz->mem);
    CloseHandle(mz->fd);
    CloseHandle(mz->mapd);

    return;
}


//
// ����������� �������� � ���������� align � ������� �������.
//
DWORD AlignToTop(DWORD value, DWORD align) {
    DWORD mask = ~ (align - 1);
    return (value + align - 1) & mask;
}


//
// ����������� �������� � ���������� align � ������ �������.
//
DWORD AlignToBottom(DWORD value, DWORD align) {
    DWORD mask = ~ (align - 1);
    return value & mask;
}


//
// ������� ���� ������ ����������������� ��������.
//
void PrintHexLine(unsigned char *line, unsigned int lineSize) {
    unsigned int i;

    // ����� hex-����� ��������
    for (i = 0; i < lineSize; ++i) {
        printf("%02X ", line[i]);
    }
    printf(" | ");

    // ����� ASCII-��������
    for (i = 0; i < lineSize; ++i) {
        if (isprint(line[i])) {
            printf("%c", line[i]);
        }
        else {
            printf(".");
        }
    }

    return;
}


//
// ������� ���� ������ � ����������������� ����.
//
void PrintSegmentDump(unsigned char *memory, unsigned int bytesFromMemoryCount, unsigned bytesTotalCount, 
                      unsigned int segmentAddress, unsigned int bytesPerLine, char *prefixName) {
    unsigned int bytesIndex;
    unsigned char *line = (unsigned char*) malloc(bytesPerLine);
    unsigned int lineIndex = 0;

    for (bytesIndex = 0; bytesIndex < bytesTotalCount; bytesIndex++) {
        // ���� ������ ������ - ������� ��� �������� � �����
        if (bytesIndex % bytesPerLine == 0) {
            printf("%s %04X:%04X ", prefixName, 
                   segmentAddress + (SIZE_OF_SEG_P * (bytesIndex / SIZE_OF_SEG_B)), bytesIndex % SIZE_OF_SEG_B);
        }
        // ������ ������ �� ������
        if (bytesIndex < bytesFromMemoryCount) {
            line[lineIndex++] = memory[bytesIndex];
        }
        // ���� ������� � ������ ����������� - ��������� ������
        else {
            line[lineIndex++] = 0;
        }
        // ���� ����� ��������� ������
        if ((bytesIndex + 1) % bytesPerLine == 0) {
            PrintHexLine(line, bytesPerLine);
            printf("\n");
            lineIndex = 0;
        }
     }

    free (line);

    return;
}


//
// ����� ���������� � DOS-���������.
//
void PrintDOSHeader(MZHeaders *mz) {
    
    printf("DOS-header info\n");
    printf("Number of bytes on last 512-byte page: %d\n", mz->doshead->e_cblp);
    printf("Total number of 512-byte pages: %d\n", mz->doshead->e_cp);
    printf("Header size in paragraphs: %d\n", mz->doshead->e_cparhdr);
    printf("Initial SS:SP value: %04X:%04X\n", mz->doshead->e_ss, mz->doshead->e_sp);
    printf("Initial CS:IP value: %04X:%04X\n", mz->doshead->e_cs, mz->doshead->e_ip);
    printf("\n");

    return;
}


//
// ����� ���������� � ������� �������
//
void PrintRelocsTable(MZHeaders *mz) {
    unsigned int i;

    printf("Relocs table info\n");
    for (i = 0; i < mz->relocs_size; i++) {
        printf("%d entry: %04X:%04X\n", i + 1, mz->relocs[i].segment, mz->relocs[i].offset);
    }
    printf("\n");

    return;
}


//
// ������� ���������� ��� ��������� � ����������������� ����.
//
void PrintSegments(MZHeaders *mz) {

    ApplyRelocs(mz, IMAGE_BASE_SEG);

    printf("Segments (with image base segment set to %04X)\n", IMAGE_BASE_SEG);
    if (mz->data_size != 0) {
        PrintSegmentDump(
            mz->data, 
            mz->data_size, 
            AlignToTop(mz->data_size, BYTES_PER_LINE), 
            IMAGE_BASE_SEG, 
            BYTES_PER_LINE, 
            "data");
    }
    if (mz->stack < mz->code) {
        if (mz->stack_size != 0) {
            PrintSegmentDump(
                mz->stack, 
                mz->stack_size, 
                AlignToTop(mz->stack_size, BYTES_PER_LINE), 
                IMAGE_BASE_SEG + mz->doshead->e_ss, 
                BYTES_PER_LINE, 
                "stck");
        }
        if (mz->code_size != 0) {
            PrintSegmentDump(
                mz->code, 
                mz->code_size, 
                AlignToTop(mz->code_size, BYTES_PER_LINE), 
                IMAGE_BASE_SEG + mz->doshead->e_cs, 
                BYTES_PER_LINE, 
                "code");
        }
    }
    else {
        if (mz->code_size != 0) {
            PrintSegmentDump(
                mz->code, 
                mz->code_size, 
                AlignToTop(mz->code_size, BYTES_PER_LINE), 
                IMAGE_BASE_SEG + mz->doshead->e_cs, 
                BYTES_PER_LINE, 
                "code");
        }
         if (mz->stack_size != 0) {
            PrintSegmentDump(
                mz->stack, 
                mz->stack_size, 
                AlignToTop(mz->stack_size, BYTES_PER_LINE), 
                IMAGE_BASE_SEG + mz->doshead->e_ss, 
                BYTES_PER_LINE, 
                "stck");
        }
    }
    printf("\n");

    RemoveRelocs(mz, IMAGE_BASE_SEG);

    return;
}