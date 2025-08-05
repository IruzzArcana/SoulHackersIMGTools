#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <tinydir.h>

typedef struct PSXCDLOC
{
    unsigned long lba_start;
    unsigned long block_size;
    unsigned long filesize;
} PSXCDLOC_t, *pPSXCDLOC;

PSXCDLOC_t PSXCDLOCArr[0x1000 / sizeof(PSXCDLOC_t)] = {0};
char PSXCDNAMArr[0x2000 / 32][32] = {0};

int CreatePSXCD(char * path)
{
    unsigned char * buffer;
    tinydir_dir dir;
    FILE * fPSXCDLOC;
    FILE * fPSXCDNAM;
    FILE * fPSXCD;
    int ret = 0;

    fPSXCDLOC = fopen("PSXCDLOC.BIN", "wb");
    fPSXCDNAM = fopen("PSXCDNAM.BIN", "wb");
    fPSXCD = fopen("PSXCD.IMG", "wb");

    if (tinydir_open(&dir, path) == -1)
    {
        printf("Failed to open %s!", path);
        return -1;
    }

    if (dir.n_files > 256)
        printf("[WARN]: directory has more than 256 files, only the first 256 files will be added\n");
    
    int i = 0;
    while (dir.has_next && i < 256)
    {
        tinydir_file file;
		if (tinydir_readfile(&dir, &file) == -1)
		{
			perror("Error getting file.");
            ret = -1;
			break;
		}

        if (file.is_dir)
        {  
            if (tinydir_next(&dir) < 0)
            {
                ret = -1;
                break;
            }
            continue;
        }

        FILE * fp;
        fp = fopen(file.path, "rb");
        fseek(fp, 0L, SEEK_END);
        int filesize = ftell(fp);

        if (filesize == 0)
        {
            printf("[WARN] %s size is zero, skipping...\n", file.name);
            fclose(fp);
            if (tinydir_next(&dir) < 0)
            {
                ret = -1;
                break;
            }
            continue;
        }
        
        PSXCDLOCArr[i].block_size = (filesize + 2048 - 1) / 2048;
        PSXCDLOCArr[i].filesize = filesize;
        if (i == 0)
            PSXCDLOCArr[i].lba_start = 0;
        else
            PSXCDLOCArr[i].lba_start = PSXCDLOCArr[i - 1].block_size + PSXCDLOCArr[i - 1].lba_start;
        
        buffer = malloc(filesize);
        rewind(fp);
        fread(buffer, 1, filesize, fp);
        fclose(fp);

        fwrite(buffer, filesize, 1, fPSXCD);
        free(buffer);
        
        char padding = 0;
        fwrite(&padding, sizeof(padding), ((filesize + 0x7FF) & ~0x7FF) - filesize, fPSXCD);

        printf("Writing %s...\n", file.path);
        strncpy(PSXCDNAMArr[i], file.name, 32);

        if (tinydir_next(&dir) < 0)
        {
            printf("Failed to get next file.\n");
            ret = -1;
            break;
        }
        i++;
    }

    fwrite(PSXCDNAMArr, sizeof(PSXCDNAMArr), 1, fPSXCDNAM);
    fwrite(PSXCDLOCArr, sizeof(PSXCDLOCArr), 1, fPSXCDLOC);

    tinydir_close(&dir);
    fclose(fPSXCDLOC);
    fclose(fPSXCDNAM);
    fclose(fPSXCD);

    return ret;
}


int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("USAGE: %s <directory>", argv[0]);
        return 1;
    }

    int ret = CreatePSXCD(argv[1]);

    return ret;
}