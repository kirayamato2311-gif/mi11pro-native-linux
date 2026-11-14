#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

VOID MarsStorageProbe (VOID)
{
  EFI_HANDLE *Handles;
  UINTN Count, Index;
  EFI_STATUS Status;
  EFI_BLOCK_IO_PROTOCOL *Block;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
  EFI_FILE_PROTOCOL *Root;
  UINT64 InfoBuffer[512];
  UINTN Size, Item;
  EFI_FILE_INFO *Info;

  gBS->SetWatchdogTimer (0, 0, 0, NULL);
  Print (L"\nMARS STORAGE PROBE - read only\n");
  Count = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &Count, &Handles);
  Print (L"Block I/O: %r, handles=%lu\n", Status, (UINT64)Count);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < Count; Index++) {
      Status = gBS->HandleProtocol (Handles[Index], &gEfiBlockIoProtocolGuid, (VOID **)&Block);
      if (!EFI_ERROR (Status) && Block->Media != NULL) {
        Print (L"B%lu present=%d partition=%d RO=%d block=%u last=%lu\n",
          (UINT64)Index, Block->Media->MediaPresent, Block->Media->LogicalPartition,
          Block->Media->ReadOnly, Block->Media->BlockSize, Block->Media->LastBlock);
      }
    }
    FreePool (Handles);
  }
  Print (L"Pausing 30 seconds for photo\n");
  gBS->Stall (30000000);
  Count = 0;
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &Count, &Handles);
  Print (L"Filesystems: %r, handles=%lu\n", Status, (UINT64)Count);
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < Count; Index++) {
      Print (L"\nFILESYSTEM %lu\n", (UINT64)Index);
      Status = gBS->HandleProtocol (Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
      if (EFI_ERROR (Status)) { Print (L"Protocol: %r\n", Status); continue; }
      Status = Fs->OpenVolume (Fs, &Root);
      Print (L"OpenVolume: %r\n", Status);
      if (!EFI_ERROR (Status)) {
        for (Item = 0; Item < 12; Item++) {
          Size = sizeof (InfoBuffer);
          Status = Root->Read (Root, &Size, InfoBuffer);
          if (EFI_ERROR (Status) || Size == 0) { Print (L"Read: %r bytes=%lu\n", Status, (UINT64)Size); break; }
          Info = (EFI_FILE_INFO *)InfoBuffer;
          Print (L"%s %s (%lu bytes)\n", (Info->Attribute & EFI_FILE_DIRECTORY) ? L"DIR" : L"FILE", Info->FileName, Info->FileSize);
        }
        Root->Close (Root);
      }
      Print (L"Pausing 15 seconds\n");
      gBS->Stall (15000000);
    }
    FreePool (Handles);
  }
  Print (L"Probe complete. Opening menu in 20 seconds.\n");
  gBS->Stall (20000000);
}
