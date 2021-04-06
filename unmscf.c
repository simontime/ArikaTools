#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#define bswap16(x) _byteswap_ushort(x)
#define bswap32(x) _byteswap_ulong(x)
#else
#define bswap16(x) __builtin_bswap16(x)
#define bswap32(x) __builtin_bswap32(x)
#endif

#define DSP_PARAM_LENGTH 0x60

typedef struct
{
    char     magic[4];
    uint16_t version;
    uint16_t paramStartOffset;
    uint16_t numEntries;
    uint16_t audioStartOffset;
} MscfHeader;

typedef struct
{
    uint16_t fieldLength;
    uint16_t unknown;
    uint32_t entryOffset;
} MscfParam;

typedef struct
{
    uint16_t fieldLength;
    uint16_t unknown;
    uint32_t audioOffset;
    uint32_t dspParamOffset;
    uint32_t audioLength;
} MscfEntry;

typedef struct
{
    const char  *filename;
    const char **contentNames;
} ContentMap;

const char *bgmNames[] =
{
    "BGM_TITLE",
    "BGM_MENU",
    "BGM_FEVER",
    "BGM_CHILL",
    "BGM_CONGRA",
    "BGM_ENDING",
    "BGM_LUIGI_1",
    "BGM_LUIGI_2",
    "BGM_SB_FEVER",
    "BGM_SB_CHILL",
    "BGM_TEST_PAN"
};

const char *jglNames[] =
{
    "JGL_GAMESTART",
    "JGL_GAMEOVER",
    "JGL_STGCLEAR_1",
    "JGL_STGCLEAR_2",
    "JGL_STGCLEAR_3",
    "JGL_STGCLEAR_SUB1",
    "JGL_STGCLEAR_SUB2",
    "JGL_STGCLEAR_SUB3",
    "JGL_VSWIN",
    "JGL_UNUSED" // ???
};

const ContentMap knownFiles[] =
{
    { "bgm.mscf", bgmNames },
    { "jgl.mscf", jglNames }
};

int main(int argc, char **argv)
{    
    if (argc != 2)
    {
        printf("Usage: %s file.mscf\n", argv[0]);
        return 0;
    }

    const ContentMap *map = NULL;
    
    // find input MCSF file in known file list
    for (uint8_t i = 0; i < sizeof(knownFiles) / sizeof(knownFiles[0]); i++)
    {
        if (strcmp(knownFiles[i].filename, argv[1]) == 0)
            map = &knownFiles[i];
    }
    
    if (map == NULL)
    {
        fprintf(stderr, "Error: Unknown file %s.\n", argv[0]);
        return 1;
    }
    
    FILE *in;
    
    if ((in = fopen(argv[1], "rb")) == NULL)
    {
        perror("Error opening file");
        return 1;
    }
    
    MscfHeader hdr;

    // read MSCF header
    fread(&hdr, 1, sizeof(MscfHeader), in);
    
    // ensure valid file magic
    if (strncmp(hdr.magic, "MSCF", 4) != 0)
    {
        fprintf(stderr, "Error: Invalid file magic %.4s.\n", hdr.magic);
        return 1;
    }
    
    const uint16_t numEntries = bswap16(hdr.numEntries) * 2;
    
    MscfEntry *entries = malloc(numEntries * sizeof(MscfEntry));
    
    // skip past pointer tables (not gonna bother parsing those) and read entries directly
    fseek(in, bswap16(hdr.paramStartOffset) +
        (numEntries / 2) * sizeof(MscfParam) + numEntries * sizeof(uint32_t), SEEK_SET);
    fread(entries, sizeof(MscfEntry), numEntries, in);
    
    for (uint16_t i = 0; i < numEntries; i++)
    {
        uint8_t dspParam[DSP_PARAM_LENGTH];

        // seek to and read DSP params
        fseek(in, bswap32(entries[i].dspParamOffset), SEEK_SET);
        fread(dspParam, 1, DSP_PARAM_LENGTH, in);
        
        uint32_t audioLength = bswap32(entries[i].audioLength);
        uint8_t *audio       = malloc(audioLength);
        
        // seek to and read audio data
        fseek(in, bswap32(entries[i].audioOffset), SEEK_SET);
        fread(audio, 1, audioLength, in);

        char filename[FILENAME_MAX];

        sprintf(filename, "%s_%c.dsp",
            map->contentNames[i / 2], ((i % 2) == 0) ? 'L' : 'R');

        printf("Saving %s...\n", filename);

        FILE *out = fopen(filename, "wb");
        
        // write .dsp file
        fwrite(dspParam, 1, DSP_PARAM_LENGTH, out);
        fwrite(audio, 1, audioLength, out);
        fclose(out);
        
        free(audio);
    }
    
    free(entries);
    
    fclose(in);
    
    puts("Done!");
    
    return 0;
}
