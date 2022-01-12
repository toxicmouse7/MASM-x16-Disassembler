/*

������������ ���� ������ mz.c.

��������� ������� ��������, ������ ���� �������������     30.04.2020

*/


#ifndef _MZ_H_
#define _MZ_H_


#include <windows.h>


#define PARAGRAPH_SIZE  16
#define BYTES_PER_LINE  16
#define IMAGE_BASE_SEG  0x1000

#define SIZE_OF_SEG_P   0x1000
#define SIZE_OF_SEG_B   0x10000

#define MZ_SIGNATURE    0x5A4D

//----------------------------------------

// ��������� ��������� DOS
typedef struct _DOS_HEADER {         
    WORD   e_magic;                 // ���������� ����� (��������� MZ)
    WORD   e_cblp;                  // ����� ���� ��������� 512-�������� �������� �����
    WORD   e_cp;                    // ����� ����� 512-�������� ������� � ����� (������ ��������� ��������)
    WORD   e_crlc;                  // ����� ��������� � ������� �������
    WORD   e_cparhdr;               // ������ ��������� � ���������� (�������� - 16 ����)
    WORD   e_minalloc;              // ����������� ����� �������������� ���������� 
    WORD   e_maxalloc;              // ������������ ����� �������������� ���������� (������ ����� 0FFFFh)
    WORD   e_ss;                    // ��������� �������� ����������� �������� SS (������������ ������ ������������ ������)
    WORD   e_sp;                    // ��������� �������� �������� SP
    WORD   e_csum;                  // ����������� ����� (��� 0 ��� ������������ � ���������� ������� � �����)
    WORD   e_ip;                    // ��������� �������� �������� IP
    WORD   e_cs;                    // ��������� �������� ����������� �������� CS (������������ ������ ������������ ������)
    WORD   e_lfarlc;                // �������� �� ������ ������� �������
    WORD   e_ovno;                  // ������� ��������� (������ ����� 0h)
} DOS_HEADER, *PDOS_HEADER;

// ��������� �������� ������� �������
typedef struct _RELOC_TABLE_ENTRY {
    WORD offset;
    WORD segment;
} RELOC_TABLE_ENTRY, *PRELOC_TABLE_ENTRY;

// ��������������� ��������� ������������ MZ-�����
typedef struct _MZHeaders {
    char                *filename;      // ��� �����

    HANDLE              fd;             // ����� ��������� �����
    HANDLE              mapd;           // ����� �������� ��������
    PBYTE               mem;            // ��������� �� ������ ���������������� �����
    DWORD               filesize;       // ������ ��������������� ����� �����

    PDOS_HEADER         doshead;        // ��������� �� DOS ���������

    PBYTE               data;           // ��������� �� ������� ������ ��������� (������ ������ ���������)
    DWORD               data_size;      // ������ �������� ������
    PBYTE               stack;          // ��������� �� ������� ����� ���������
    DWORD               stack_size;     // ������ �������� �����
    PBYTE               code;           // ��������� �� ������� ���� ���������
    DWORD               code_size;      // ������ �������� ����

    PRELOC_TABLE_ENTRY  relocs;         // ��������� �� ������ ������� �������
    WORD                relocs_size;    // ������ ������� �������
} MZHeaders;

BOOL LoadMZFile(char *filename, MZHeaders *mz);

void UnloadMZFile(MZHeaders *mz);

void ApplyRelocs(MZHeaders *mz, WORD imageBase);

void RemoveRelocs(MZHeaders *mz, WORD imageBase);

void PrintDOSHeader(MZHeaders *mz);

void PrintRelocsTable(MZHeaders *mz);

void PrintSegments(MZHeaders *mz);

//----------------------------------------

#endif  // _MZ_H_