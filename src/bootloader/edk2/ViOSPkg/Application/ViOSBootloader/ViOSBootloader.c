#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/FileInfo.h>
#include "config.h"
#include <Library/BaseMemoryLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

typedef struct __attribute__((packed))
{
  UINT64 base_addr;
  UINT64 length;
  UINT32 type;
  UINT32 acpi_attrs;
} E820Entry;

static EFI_STATUS GetMemoryMapSnapshot(
  EFI_MEMORY_DESCRIPTOR **MemoryMapOut,
  UINTN *MemoryMapSizeOut,
  UINTN *MapKeyOut,
  UINTN *DescriptorSizeOut,
  UINT32 *DescriptorVersionOut
)
{
  EFI_STATUS Status = gBS->GetMemoryMap(
    MemoryMapSizeOut,
    NULL,
    MapKeyOut,
    DescriptorSizeOut,
    DescriptorVersionOut
  );

  if (Status != EFI_BUFFER_TOO_SMALL && EFI_ERROR(Status))
  {
    return Status;
  }

  *MemoryMapOut = AllocatePool(*MemoryMapSizeOut + (*DescriptorSizeOut * 2));
  if (*MemoryMapOut == NULL)
  {
    return EFI_OUT_OF_RESOURCES;
  }

  return gBS->GetMemoryMap(
    MemoryMapSizeOut,
    *MemoryMapOut,
    MapKeyOut,
    DescriptorSizeOut,
    DescriptorVersionOut
  );
}

static UINTN CountConventionalDescriptors(
  EFI_MEMORY_DESCRIPTOR *MemoryMap,
  UINTN MemoryMapSize,
  UINTN DescriptorSize
)
{
  UINTN Total = 0;
  EFI_MEMORY_DESCRIPTOR *Descriptor = MemoryMap;
  UINTN DescriptorCount = MemoryMapSize / DescriptorSize;

  for (UINTN Index = 0; Index < DescriptorCount; ++Index)
  {
    if (Descriptor->Type == EfiConventionalMemory)
    {
      ++Total;
    }

    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  return Total;
}

static EFI_STATUS WriteE820Map(
  EFI_MEMORY_DESCRIPTOR *MemoryMap,
  UINTN MemoryMapSize,
  UINTN DescriptorSize
)
{
  E820Entry *Entries = (E820Entry *)(UINTN)MEMORY_MAP_LOCATION;
  UINT16 Count = 0;
  EFI_MEMORY_DESCRIPTOR *Descriptor = MemoryMap;
  UINTN DescriptorCount = MemoryMapSize / DescriptorSize;

  for (UINTN Index = 0; Index < DescriptorCount; ++Index)
  {
    if (Descriptor->Type == EfiConventionalMemory)
    {
      E820Entry *Entry = &Entries[Count];
      Entry->base_addr = Descriptor->PhysicalStart;
      Entry->length = Descriptor->NumberOfPages * EFI_PAGE_SIZE;
      Entry->type = 1;
      Entry->acpi_attrs = 0;
      ++Count;
    }

    Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptor + DescriptorSize);
  }

  *((UINT16 *)(UINTN)MEMORY_MAP_TOTAL_ENTRIES_LOCATION) = Count;
  return EFI_SUCCESS;
}

static EFI_STATUS ReadFileFromCurrentFilesystem(CHAR16 *FileName, VOID **BufferOut, UINTN *BufferSizeOut)
{
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL* LoadedImageProtocol = NULL;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* SimpleFileSystem = NULL;

  EFI_FILE_PROTOCOL* Root = NULL;
  EFI_FILE_PROTOCOL* File = NULL;
  UINTN FileInfoSize = 0;

  *BufferOut = NULL;
  *BufferSizeOut = 0;

  Status = gBS->HandleProtocol(
        gImageHandle,
      &gEfiLoadedImageProtocolGuid,
      (VOID**)&LoadedImageProtocol
  );

  if (EFI_ERROR(Status))
  {
     return Status;
  }

  Status = gBS->HandleProtocol(
      LoadedImageProtocol->DeviceHandle,
      &gEfiSimpleFileSystemProtocolGuid,
      (VOID**)&SimpleFileSystem
  );

  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Status = Root->Open(
    Root,
    &File,
    FileName,
    EFI_FILE_MODE_READ,
    0
  );

  if (EFI_ERROR(Status))
  {
    return Status;
  }

  FileInfoSize = OFFSET_OF(EFI_FILE_INFO, FileName) + 256 * sizeof(CHAR16);
  VOID* FileInfoBuffer = AllocatePool(FileInfoSize);
  if (FileInfoBuffer == NULL)
  {
    File->Close(File);
    return EFI_OUT_OF_RESOURCES;
  }

  EFI_FILE_INFO* FileInfo = (EFI_FILE_INFO*)FileInfoBuffer;
  Status = File->GetInfo(
    File, 
    &gEfiFileInfoGuid, 
    &FileInfoSize,
    FileInfo
  );

  if (EFI_ERROR(Status))
  {
    FreePool(FileInfoBuffer);
    File->Close(File);
    return Status;
  }

  UINTN BufferSize = FileInfo->FileSize;
  FreePool(FileInfoBuffer);
  FileInfoBuffer = NULL;

  VOID* Buffer = AllocatePool(BufferSize);
  if(Buffer == NULL)
  {
    File->Close(File);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = File->Read(File, &BufferSize, Buffer);
  if (EFI_ERROR(Status))
  {
    FreePool(Buffer);
    File->Close(File);
    return Status;
  }

  *BufferOut = Buffer;
  *BufferSizeOut = BufferSize;
  File->Close(File);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  (void)SystemTable;
  gImageHandle = ImageHandle;

  VOID *KernelBuffer = NULL;
  UINTN KernelBufferSize = 0;
  EFI_STATUS Status = ReadFileFromCurrentFilesystem(L"kernel.bin", &KernelBuffer, &KernelBufferSize);
  if (EFI_ERROR(Status))
  {
     return Status;
  }

  EFI_PHYSICAL_ADDRESS KernelBase = KERNEL_LOCATION;
  Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, EFI_SIZE_TO_PAGES(KernelBufferSize), &KernelBase);
  if (EFI_ERROR(Status))
  {
    FreePool(KernelBuffer);
    return Status;
  }

  CopyMem((VOID *)(UINTN)KernelBase, KernelBuffer, KernelBufferSize);
  FreePool(KernelBuffer);

  EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
  UINTN MemoryMapSize = 0;
  UINTN MapKey = 0;
  UINTN DescriptorSize = 0;
  UINT32 DescriptorVersion = 0;

  Status = GetMemoryMapSnapshot(&MemoryMap, &MemoryMapSize, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  UINTN ConventionalDescriptors = CountConventionalDescriptors(MemoryMap, MemoryMapSize, DescriptorSize);
  UINTN E820Size = ConventionalDescriptors * sizeof(E820Entry);
  EFI_PHYSICAL_ADDRESS E820Base = MEMORY_MAP_LOCATION;
  Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, EFI_SIZE_TO_PAGES(E820Size), &E820Base);
  if (EFI_ERROR(Status))
  {
    FreePool(MemoryMap);
    return Status;
  }

  FreePool(MemoryMap);

  Status = GetMemoryMapSnapshot(&MemoryMap, &MemoryMapSize, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Status = WriteE820Map(MemoryMap, MemoryMapSize, DescriptorSize);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Status = gBS->ExitBootServices(ImageHandle, MapKey);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  __asm__ __volatile__("jmp *%0" : : "r"((VOID *)(UINTN)(KernelBase + KERNEL_UEFI_ENTRY_OFFSET)));
  return EFI_SUCCESS;
}