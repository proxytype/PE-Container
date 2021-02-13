
#include <iostream>
#include <windows.h>

const char* PREFIX_INDEX = ".inx";
const char* PREFIX_SECTION = ".x";

HANDLE hConsole = NULL;
WORD attributes = 0;

int split(char* str, char delim, char*** array, int* length) {
    char* p;
    char** res;
    int count = 0;
    int k = 0;

    p = str;
    // Count occurance of delim in string
    while ((p = strchr(p, delim)) != NULL) {
        *p = 0; // Null terminate the deliminator.
        p++; // Skip past our new null
        count++;
    }

    // allocate dynamic array
    res = (char**)calloc(1, count * sizeof(char*));
    if (!res) return -1;

    p = str;
    for (k = 0; k < count; k++) {
        if (*p) res[k] = p;  // Copy start of string
        p = strchr(p, 0);    // Look for next null
        p++; // Start of next string
    }

    *array = res;
    *length = count;

    return 0;
}

void printheader() {
    printf("     ____  ______   ______            __        _                \n");
    printf("    / __ \\/ ____/  / ____/___  ____  / /_____ _(_)___  ___  _____\n");
    printf("   / /_/ / __/    / /   / __ \\/ __ \\/ __/ __ `/ / __ \\/ _ \\/ ___/\n");
    printf("  / ____/ /___   / /___/ /_/ / / / / /_/ /_/ / / / / /  __/ /    \n");
    printf(" /_/   /_____/   \\____/\\____/_/ /_/\\__/\\__,_/_/_/ /_/\\___/_/     \n");
    printf(" -----------------------------------------------------------\n");
    printf(" PEExtractor - Rudenetworks.com version: 1.0 alpha\n");
}

int main()
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &Info);
    attributes = Info.wAttributes;

    SetConsoleTextAttribute(hConsole,
        FOREGROUND_BLUE | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    printheader();
    printf(" - Start to extract files.\n");

    HANDLE mn = GetModuleHandle(NULL);
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)mn;

    if (dosHeader == NULL || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {

        //TODO:nothing?
        return 0;
    }

    PIMAGE_NT_HEADERS ntHeader = (PIMAGE_NT_HEADERS)((PUCHAR)mn + dosHeader->e_lfanew);
    PIMAGE_FILE_HEADER fileHeader = (PIMAGE_FILE_HEADER)((PUCHAR)mn + dosHeader->e_lfanew + sizeof(DWORD));
    PIMAGE_OPTIONAL_HEADER optionalHeader = (PIMAGE_OPTIONAL_HEADER)((PUCHAR)mn + dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
    PIMAGE_SECTION_HEADER sectionHeader = (PIMAGE_SECTION_HEADER)((PUCHAR)mn + dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));

    PIMAGE_SECTION_HEADER indexSection = (PIMAGE_SECTION_HEADER)&sectionHeader[fileHeader->NumberOfSections - 1];
    char sectionName[8] = { 0 };
    memcpy(sectionName, indexSection->Name, 8);
    if (strcmp(sectionName, PREFIX_INDEX) == 0) {
        printf("- Found index section.\n");

        char *files = new char[indexSection->SizeOfRawData];

        //memcpy_s(indexBuffer, indexSection->SizeOfRawData, (PVOID)((LPBYTE)mn + indexSection->VirtualAddress), indexSection->SizeOfRawData);
        memcpy(files, (PVOID)((LPBYTE)mn + indexSection->VirtualAddress), indexSection->SizeOfRawData);
        
        printf(" - Detected files:\n");

        char** res;
        int count = 0;
        int rc;

        rc = split(files, ';', &res, &count);

        int localfileIndex = 0;
        for (int i = 0; i < fileHeader->NumberOfSections; i++)
        {
            PIMAGE_SECTION_HEADER s = (PIMAGE_SECTION_HEADER)&sectionHeader[i];
            char sectionName[8] = { 0 };
            memcpy(sectionName, s->Name, 8);

            char* f = strstr(sectionName, PREFIX_SECTION);
            if (f != NULL) {

                byte* buffer = (byte*)malloc(s->SizeOfRawData);

                printf(" - Extract section: %s\n", sectionName);

                memcpy(buffer, (PVOID)((LPBYTE)mn + s->VirtualAddress), s->SizeOfRawData);

                HANDLE h = CreateFile(res[localfileIndex], GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                if (h != INVALID_HANDLE_VALUE) {
                    WriteFile(h, buffer, s->SizeOfRawData, NULL, NULL);
                    CloseHandle(h);
                    printf("  + %s, size:%d bytes.\n", res[localfileIndex], s->SizeOfRawData);
                }
                else {
                    printf(" - Failed write:%s.\n", res[localfileIndex]);
                }

                localfileIndex += 1;
            }
        }

        free(res);
    }

    printf(" - Extract complete.\n");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attributes);

    return 0;
}
