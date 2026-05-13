#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#include <Guid/FileInfo.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/GraphicsOutput.h>

#include "config.h"

typedef struct __attribute__((packed))
{
    UINT64 base_addr;
    UINT64 length;
    UINT32 type;
    UINT32 acpi_attrs;
} E820Entry;

static EFI_STATUS ReadFileFromCurrentFilesystem(
    CHAR16 *FileName,
    VOID **BufferOut,
    UINTN *BufferSizeOut)
{
    EFI_STATUS Status;

    EFI_LOADED_IMAGE_PROTOCOL *LoadedImageProtocol = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem = NULL;

    EFI_FILE_PROTOCOL *Root = NULL;
    EFI_FILE_PROTOCOL *File = NULL;

    *BufferOut = NULL;
    *BufferSizeOut = 0;

    Status = gBS->HandleProtocol(
        gImageHandle,
        &gEfiLoadedImageProtocolGuid,
        (VOID **)&LoadedImageProtocol);

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    Status = gBS->HandleProtocol(
        LoadedImageProtocol->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID **)&SimpleFileSystem);

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    Status = SimpleFileSystem->OpenVolume(
        SimpleFileSystem,
        &Root);

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    Status = Root->Open(
        Root,
        &File,
        FileName,
        EFI_FILE_MODE_READ,
        0);

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    UINTN FileInfoSize =
        SIZE_OF_EFI_FILE_INFO + 256;

    EFI_FILE_INFO *FileInfo =
        AllocatePool(FileInfoSize);

    if (FileInfo == NULL)
    {
        File->Close(File);
        return EFI_OUT_OF_RESOURCES;
    }

    Status = File->GetInfo(
        File,
        &gEfiFileInfoGuid,
        &FileInfoSize,
        FileInfo);

    if (EFI_ERROR(Status))
    {
        FreePool(FileInfo);
        File->Close(File);
        return Status;
    }

    UINTN BufferSize = FileInfo->FileSize;

    FreePool(FileInfo);

    VOID *Buffer = AllocatePool(BufferSize);
    if (Buffer == NULL)
    {
        File->Close(File);
        return EFI_OUT_OF_RESOURCES;
    }

    Status = File->Read(
        File,
        &BufferSize,
        Buffer);

    File->Close(File);

    if (EFI_ERROR(Status))
    {
        FreePool(Buffer);
        return Status;
    }

    *BufferOut = Buffer;
    *BufferSizeOut = BufferSize;

    return EFI_SUCCESS;
}

static EFI_STATUS WriteE820Map(
    EFI_MEMORY_DESCRIPTOR *MemoryMap,
    UINTN MemoryMapSize,
    UINTN DescriptorSize)
{
    E820Entry *Entries =
        (E820Entry *)(UINTN)MEMORY_MAP_LOCATION;

    UINT16 Count = 0;

    EFI_MEMORY_DESCRIPTOR *Descriptor = MemoryMap;

    UINTN DescriptorCount =
        MemoryMapSize / DescriptorSize;

    for (UINTN Index = 0;
         Index < DescriptorCount;
         ++Index)
    {
        if (Descriptor->Type == EfiConventionalMemory)
        {
            E820Entry *Entry = &Entries[Count];

            Entry->base_addr =
                Descriptor->PhysicalStart;

            Entry->length =
                Descriptor->NumberOfPages * EFI_PAGE_SIZE;

            Entry->type = 1;
            Entry->acpi_attrs = 0;

            ++Count;
        }

        Descriptor =
            (EFI_MEMORY_DESCRIPTOR *)(
                (UINT8 *)Descriptor + DescriptorSize);
    }

    *((UINT16 *)(UINTN)
          MEMORY_MAP_TOTAL_ENTRIES_LOCATION) = Count;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    (void)SystemTable;

    gImageHandle = ImageHandle;

    EFI_STATUS Status;

    /*
     * =========================================================
     * LOAD KERNEL
     * =========================================================
     */

    VOID *KernelBuffer = NULL;
    UINTN KernelBufferSize = 0;

    Status = ReadFileFromCurrentFilesystem(
        L"kernel.bin",
        &KernelBuffer,
        &KernelBufferSize);

    if (EFI_ERROR(Status))
    {
        Print(L"Failed to load kernel: %r\n", Status);
        return Status;
    }

    EFI_PHYSICAL_ADDRESS KernelBase =
        KERNEL_LOCATION;

    Status = gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(KernelBufferSize),
        &KernelBase);

    if (EFI_ERROR(Status))
    {
        Print(L"AllocatePages failed: %r\n", Status);
        FreePool(KernelBuffer);
        return Status;
    }

    CopyMem(
        (VOID *)(UINTN)KernelBase,
        KernelBuffer,
        KernelBufferSize);

    FreePool(KernelBuffer);

    /*
     * =========================================================
     * GOP
     * =========================================================
     */

    EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput =
        NULL;

    Status = gBS->LocateProtocol(
        &gEfiGraphicsOutputProtocolGuid,
        NULL,
        (VOID **)&GraphicsOutput);

    if (EFI_ERROR(Status))
    {
        Print(L"Failed to locate GOP: %r\n", Status);
        return Status;
    }

    VOID *FrameBuffer =
        (VOID *)(UINTN)
            GraphicsOutput->Mode->FrameBufferBase;

    UINTN PixelsPerScanLine =
        GraphicsOutput->Mode->Info->PixelsPerScanLine;

    UINTN HorizontalResolution =
        GraphicsOutput->Mode->Info->HorizontalResolution;

    UINTN VerticalResolution =
        GraphicsOutput->Mode->Info->VerticalResolution;

    Print(L"Framebuffer: %p\n", FrameBuffer);
    Print(L"Resolution: %ux%u\n",
          HorizontalResolution,
          VerticalResolution);

    /*
     * =========================================================
     * MEMORY MAP BUFFER
     * =========================================================
     */

    UINTN MemoryMapSize = 0;
    UINTN MapKey = 0;
    UINTN DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;

    Status = gBS->GetMemoryMap(
        &MemoryMapSize,
        NULL,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion);

    if (Status != EFI_BUFFER_TOO_SMALL)
    {
        Print(L"Initial GetMemoryMap failed: %r\n",
              Status);
        return Status;
    }

    MemoryMapSize += DescriptorSize * 8;

    EFI_MEMORY_DESCRIPTOR *MemoryMap =
        AllocatePool(MemoryMapSize);

    if (MemoryMap == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    /*
     * =========================================================
     * E820 STORAGE
     * =========================================================
     */

    UINTN MaxEntries =
        MemoryMapSize / DescriptorSize;

    UINTN E820Size =
        MaxEntries * sizeof(E820Entry);

    EFI_PHYSICAL_ADDRESS E820Base =
        MEMORY_MAP_LOCATION;

    Status = gBS->AllocatePages(
        AllocateAddress,
        EfiLoaderData,
        EFI_SIZE_TO_PAGES(E820Size),
        &E820Base);

    if (EFI_ERROR(Status))
    {
        Print(L"E820 allocation failed: %r\n",
              Status);

        return Status;
    }

    /*
     * =========================================================
     * FINAL MEMORY MAP
     * =========================================================
     */

    Status = gBS->GetMemoryMap(
        &MemoryMapSize,
        MemoryMap,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion);

    if (EFI_ERROR(Status))
    {
        Print(L"Final GetMemoryMap failed: %r\n",
              Status);

        return Status;
    }

    Status = WriteE820Map(
        MemoryMap,
        MemoryMapSize,
        DescriptorSize);

    if (EFI_ERROR(Status))
    {
        Print(L"WriteE820Map failed\n");
        return Status;
    }

    /*
     * =========================================================
     * EXIT BOOT SERVICES
     * =========================================================
     */

    Status = gBS->ExitBootServices(
        ImageHandle,
        MapKey);

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    /*
     * =========================================================
     * JUMP TO KERNEL
     * =========================================================
     */

    __asm__ __volatile__(
        "jmp *%%rax\n\t"
        :
        : "D"((UINT64)FrameBuffer),
          "S"((UINT64)PixelsPerScanLine),
          "d"((UINT64)HorizontalResolution),
          "c"((UINT64)VerticalResolution),
          "a"((UINT64)(
              KernelBase +
              KERNEL_UEFI_ENTRY_OFFSET))
        : "memory");

    __builtin_unreachable();

    return EFI_SUCCESS;
}