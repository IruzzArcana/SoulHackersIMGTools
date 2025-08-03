#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>

typedef struct PSXCDLOC
{
    unsigned long lba_start;
    unsigned long block_size;
    unsigned long filesize;
} PSXCDLOC_t, *pPSXCDLOC;

PSXCDLOC_t PSXCDLOCArr[0x1000 / sizeof(PSXCDLOC_t)] = {0};
char PSXCDNAMArr[0x2000 / 32][32] = {0};

int LoadPSXCDLookup()
{
    FILE * pfloc;
    FILE * pfnam;

    pfloc = fopen("PSXCDLOC.BIN", "rb");
    
    if (pfloc == NULL)
    {
        printf("Failed to open PSXCDLOC.BIN!\n");
        return 1;
    }

    fread(PSXCDLOCArr, sizeof(PSXCDLOCArr), 1, pfloc);
    fclose(pfloc);

    pfnam = fopen("PSXCDNAM.BIN", "rb");

    if (pfnam == NULL)
    {
        printf("Failed to open PSXCDNAM.BIN!\n");
        return 1;
    }

    fread(PSXCDNAMArr, sizeof(PSXCDNAMArr), 1, pfnam);
    fclose(pfnam);

    return 0;
}

int ExtractPSXIMG()
{
    FILE * pfPSXIMG;
    pfPSXIMG = fopen("PSXCD.IMG", "rb");

    if (pfPSXIMG == NULL)
    {
        printf("Failed to open PSXCD.IMG!\n");
        return 1;
    }
    
 #ifdef _WIN32
    if (_mkdir("PSXCD"))
#else
    if (mkdir("PSXCD", 0755))
#endif
    {
        printf("Failed to create output folder!\n");
        fclose(pfPSXIMG);
        return 1;
    }

    for (int i = 0; i < sizeof(PSXCDLOCArr) / sizeof(PSXCDLOCArr[0]); i++)
    {
        if (memcmp(&PSXCDLOCArr[i], &(PSXCDLOC_t){0}, sizeof(PSXCDLOC_t)) == 0)
            break;

        FILE * fp;
        int size = PSXCDLOCArr[i].filesize & 0x7ff;
        if (size == 0 && PSXCDLOCArr[i].block_size != 0)
            size = 0x800;
        
        int file_size = (PSXCDLOCArr[i].block_size - 1) * 0x800 + size;

        // block_size for DUMMY.DAT is 0...
        if (file_size < 0)
            file_size = 0;
            
        char fname[32];
        strncpy(fname, PSXCDNAMArr[i], 32);
        fname[31] = '\0';

        printf("Writing %s...\n", fname);
        
        char outpath[48];

        snprintf(outpath, sizeof(outpath), "PSXCD/%s", fname);
        
        fp = fopen(outpath, "wb");

        if (fp == NULL)
        {
            printf("Failed to open %s for writing, aborted.\n", outpath);
            break;
        }

        unsigned char * buffer = malloc(file_size);

        if (!buffer) {
            printf("Failed to allocate memory!\n");
            fclose(fp);
            break;
        }

        int offset = PSXCDLOCArr[i].lba_start * 0x800;

        fseek(pfPSXIMG, offset , SEEK_SET);
        
        if (fread(buffer, 1, file_size, pfPSXIMG) != file_size)
        {
            printf("Error reading from PSXCD.IMG at offset 0x%X\n", offset);
            free(buffer);
            fclose(fp);
            break;
        }

        fwrite(buffer, 1, file_size, fp);
        fclose(fp);
        free(buffer);    
    }

    fclose(pfPSXIMG);

    return 0;
}

int main()
{
    int res = LoadPSXCDLookup();

    if (res != 0)
        return 1;
    
    ExtractPSXIMG();

    return 0;
}