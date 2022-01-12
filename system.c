/*

������ ��������������� ��������� �������.

��������� ������� ��������, ������ ���� �������������     30.04.2020

*/


#include <windows.h>
#include <stdio.h>

#include "system.h"


//
// ���������� ������ ��������� �� �������.
//
void PrintErrorMessage (unsigned int err) {
    LPSTR msg;

    DWORD res = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
        NULL,
        err,    // ��� ������
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),  // ������������� ����������� �����
        (LPSTR) &msg,
        0,
        NULL);

    if (res != 0) {
        printf("%s", msg);
        LocalFree(msg);    // ������������ ������ � ������� ���������
    }

    return;
}


//
// ���������� ������ ��������� � ��������� ������.
//
void PrintLastErrorMessage() {

    PrintErrorMessage (GetLastError());

    return;
}