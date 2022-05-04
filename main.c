#include <windows.h>
#include <stdio.h> 

#include "disas.h"
#include "mz.h"


//
// Ïîäñêàçêà ïî èñïîëüçîâàíèþ.
//
void usage(void) {

    printf("mzinfo <filename> <opt>\n");
    printf("opt:\n"
           "\theader\n"
           "\trelocs\n"
           "\tsegments\n"
           "\tinfo\n"
           "\tdisas\n"
           "\all\n");

    return;
}


//
// Îñíîâíàÿ ôóíêöèÿ ïðîãðàììû.
//
int main(unsigned int argc, char *argv[], char *envp[]) {
    MZHeaders mz;
    char *filename;

    if(argc < 3){
        usage();
        return 1;
    }

    filename = argv[1];

    if (LoadMZFile(filename, &mz) == FALSE) {
        return 1;
    }

    printf("Information about %s\n\n", filename);

    if (!strcmp (argv[2], "header")) {
        PrintDOSHeader(&mz);
    }

    if (!strcmp (argv[2], "relocs")) {
        PrintRelocsTable(&mz);
    }

    if (!strcmp (argv[2], "segments")) {
        PrintSegments(&mz);
    }

    if (!strcmp (argv[2], "info")) {
        PrintDOSHeader(&mz);
        PrintRelocsTable(&mz);
        PrintSegments(&mz);
    }

    if (!strcmp (argv[2], "disas")) {
        DisasCodeSeg(&mz);
    }

    if (!strcmp (argv[2], "all")) {
        PrintDOSHeader(&mz);
        PrintRelocsTable(&mz);
        PrintSegments(&mz);
        DisasCodeSeg(&mz);
    }

    UnloadMZFile(&mz);

    return 0; 
}
