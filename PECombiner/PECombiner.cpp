// PECombiner.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <string.h>

const int MAX_FILEPATH_LENGTH = 255;
const char* PREFIX_INDEX = ".inx";
const char* PREFIX_SECTION_INDEX = ".x";

typedef struct PEContainer {
    HANDLE handle;
    BYTE* buffer;
    DWORD size;
    char* sectionName;
    char* fileName;
};

typedef struct PEHeader {
    HANDLE handle;
    BYTE* buffer;
    DWORD size;
    PIMAGE_DOS_HEADER dosHeader;
    PIMAGE_NT_HEADERS ntHeader;
    PIMAGE_OPTIONAL_HEADER optionalHeader;
    PIMAGE_FILE_HEADER fileHeader;
    PIMAGE_SECTION_HEADER sectionHeader;
    bool loaded;
};

HANDLE hConsole = NULL;
WORD attributes = 0;


void printheader() {
    printf("     ____  ______   ______            __        _                \n");
    printf("    / __ \\/ ____/  / ____/___  ____  / /_____ _(_)___  ___  _____\n");
    printf("   / /_/ / __/    / /   / __ \\/ __ \\/ __/ __ `/ / __ \\/ _ \\/ ___/\n");
    printf("  / ____/ /___   / /___/ /_/ / / / / /_/ /_/ / / / / /  __/ /    \n");
    printf(" /_/   /_____/   \\____/\\____/_/ /_/\\__/\\__,_/_/_/ /_/\\___/_/     \n");
    printf(" -----------------------------------------------------------\n");
    printf(" PECombiner - Rudenetworks.com version: 1.0 alpha\n");
} 
void printMenu() {
    printheader();
    printf(" PECombiner.exe <PEExtractor.exe> <Output.exe> <Files>\n");
}

char* getFileName(char* path)
{
    char* filename = strrchr(path, '\\');
    if (filename == NULL)
        filename = path;
    else
        filename++;

    return filename;
}

DWORD align(DWORD size, DWORD align, DWORD addr) {
    if (!(size % align))
        return addr + size;
    return addr + (size / align + 1) * align;
}

PIMAGE_SECTION_HEADER addSection(PEHeader* peHeader, DWORD sectionSize, const char* sectionName) {

    PIMAGE_SECTION_HEADER before = (PIMAGE_SECTION_HEADER)&peHeader->sectionHeader[peHeader->fileHeader->NumberOfSections - 1];
    PIMAGE_SECTION_HEADER next = (PIMAGE_SECTION_HEADER)&peHeader->sectionHeader[peHeader->fileHeader->NumberOfSections];

    ZeroMemory(next, sizeof(IMAGE_SECTION_HEADER));
    CopyMemory(next->Name, sectionName, 8);

    next->Misc.VirtualSize = align(sectionSize, peHeader->optionalHeader->SectionAlignment, 0);
    next->VirtualAddress = align(before->Misc.VirtualSize, peHeader->optionalHeader->SectionAlignment, before->VirtualAddress);
    next->SizeOfRawData = align(sectionSize, peHeader->optionalHeader->FileAlignment, 0);
    next->PointerToRawData = align(before->SizeOfRawData, peHeader->optionalHeader->FileAlignment, before->PointerToRawData);
    //execute section
    next->Characteristics = 0x48000000;

    peHeader->fileHeader->NumberOfSections += 1;
    peHeader->optionalHeader->SizeOfImage = next->VirtualAddress + next->Misc.VirtualSize;

    return next;
}

PEHeader peLoad(const char* arg) {

    char file[MAX_FILEPATH_LENGTH];
    memcpy_s(&file, MAX_FILEPATH_LENGTH, arg, MAX_FILEPATH_LENGTH);

    PEHeader header;

    header.handle = CreateFileA(file, GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (header.handle == INVALID_HANDLE_VALUE) {
        header.loaded = false;
        return header;
    }

    header.size = GetFileSize(header.handle, NULL);
    header.buffer = new BYTE[header.size];
    header.loaded = ReadFile(header.handle, header.buffer, header.size, NULL, NULL);

    header.dosHeader = (PIMAGE_DOS_HEADER)header.buffer;

    if (header.dosHeader == NULL || header.dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        header.loaded = false;
        return header;
    }

    header.ntHeader = (PIMAGE_NT_HEADERS)((PUCHAR)header.buffer + header.dosHeader->e_lfanew);
    header.fileHeader = (PIMAGE_FILE_HEADER)(header.buffer + header.dosHeader->e_lfanew + sizeof(DWORD));
    header.optionalHeader = (PIMAGE_OPTIONAL_HEADER)(header.buffer + header.dosHeader->e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
    header.sectionHeader = (PIMAGE_SECTION_HEADER)(header.buffer + header.dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS));

    return header;

}

int createContainer(char* extractorFile, char* output, int indexSectionSize) {

    PEHeader extractor = peLoad(extractorFile);
    PIMAGE_SECTION_HEADER sectionAdded = addSection(&extractor, indexSectionSize, PREFIX_SECTION_INDEX);

    HANDLE h = CreateFile(output, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    WriteFile(h, extractor.buffer, extractor.optionalHeader->SizeOfImage, NULL, NULL);
    SetFilePointer(h, sectionAdded->PointerToRawData + sectionAdded->SizeOfRawData, NULL, FILE_BEGIN);
    SetEndOfFile(h);

    CloseHandle(h);
    return 0;
}

int main(char argc, char* argv[])
{
    CONSOLE_SCREEN_BUFFER_INFO Info;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsole, &Info);
    attributes = Info.wAttributes;

    SetConsoleTextAttribute(hConsole,
        FOREGROUND_BLUE | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    if (argc < 3) {
        printMenu();
    }
    else {

        printheader();

        char* extractorFile = argv[1];
        char* output = argv[2];

        PEHeader extractor = peLoad(extractorFile);
        printf(" - Load extractor complete.\n");
        if (!extractor.loaded) {
            //TODO: print error
            return -1;
        }

        DWORD finalSize = extractor.size;

        DWORD totalContainers = argc - 3;
        PEContainer* containers = new PEContainer[totalContainers];

        printf(" - Creating %d containers.\n", totalContainers);

        DWORD totalIndexSectionSize = 0;
        int localIndex = 0;

        for (DWORD i = 3; i < argc; i++)
        {
            char* f = getFileName(argv[i]);
            DWORD fl = strlen(f) + 2;

            containers[localIndex].fileName = new char[fl];

            sprintf_s(containers[localIndex].fileName, fl , "%s;", f);
            

            containers[localIndex].handle = CreateFileA(argv[i], GENERIC_ALL, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (containers[localIndex].handle == INVALID_HANDLE_VALUE) {
                printf(" - Could not read file: %s\n", containers[localIndex].fileName);
                return -1;
            }

            containers[localIndex].size = GetFileSize(containers[localIndex].handle, NULL);
            containers[localIndex].sectionName = new char[8];
            sprintf_s(containers[localIndex].sectionName, 8, "%s%d", PREFIX_SECTION_INDEX, localIndex);
            finalSize += containers[localIndex].size;
            totalIndexSectionSize += strlen(containers[localIndex].fileName) + 1;
            containers[localIndex].buffer = new BYTE[containers[localIndex].size];
            bool read = ReadFile(containers[localIndex].handle, containers[localIndex].buffer, containers[localIndex].size, NULL, NULL);
            CloseHandle(containers[localIndex].handle);
            localIndex += 1;
        }

        HANDLE h = CreateFile(output, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        WriteFile(h, NULL, finalSize + totalIndexSectionSize, NULL, NULL);

        char* indexBuffer = new char[totalIndexSectionSize];
        ZeroMemory(indexBuffer, totalIndexSectionSize);
        char currentIndex = 0;
        PIMAGE_SECTION_HEADER sectionAdded = NULL;
        for (DWORD i = 0; i < totalContainers; i++)
        {
            PEContainer container = containers[i];
            sectionAdded = addSection(&extractor, container.size, container.sectionName);

            printf(" - Create section: %s.\n", container.sectionName);

            SetFilePointer(h, sectionAdded->PointerToRawData, NULL, FILE_BEGIN);
            WriteFile(h, container.buffer, containers[i].size, NULL, NULL);

            printf(" - File: %s Write section: %d.\n", containers[i].fileName, containers[i].size);

            strcpy_s(&indexBuffer[currentIndex], strlen(container.fileName) + 1, container.fileName);
            currentIndex += strlen(containers[i].fileName);
            
        }

        printf(" - Create index section.\n");
        sectionAdded = addSection(&extractor, totalIndexSectionSize, PREFIX_INDEX);
        SetFilePointer(h, sectionAdded->PointerToRawData, NULL, FILE_BEGIN);
        WriteFile(h, indexBuffer, totalIndexSectionSize, NULL, NULL);

        SetFilePointer(h, sectionAdded->PointerToRawData + sectionAdded->SizeOfRawData, NULL, FILE_BEGIN);
        SetEndOfFile(h);
        printf(" - Set end of file.\n");
        SetFilePointer(h, 0, NULL, FILE_BEGIN);
        WriteFile(h, extractor.buffer, extractor.size, NULL, NULL);
        printf(" - Write updated extractor.\n");
        CloseHandle(h);

        printf(" - %s is ready.\n", output);
    }

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), attributes);

    return 0;
}