#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Library/HobLib.h>
#include <Library/FdtLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Protocol/RamDisk.h>

STATIC UINT64 ReadInitrdProperty (VOID *Dtb, INT32 Node, CONST CHAR8 *Name)
{
  INT32 Length;
  CONST UINT32 *Value = FdtGetProp (Dtb, Node, Name, &Length);
  if (Value == NULL || (Length != 4 && Length != 8)) return 0;
  if (Length == 4) return Fdt32ToCpu (Value[0]);
  return ((UINT64)Fdt32ToCpu (Value[0]) << 32) | Fdt32ToCpu (Value[1]);
}

STATIC VOID CheckInitrd (UINTN Address)
{
  VOID *Dtb = (VOID *)Address;
  INT32 Node;
  UINT64 Start, End;
  EFI_RAM_DISK_PROTOCOL *RamDisk;
  EFI_DEVICE_PATH_PROTOCOL *DiskPath;
  EFI_STATUS Status;
  STATIC EFI_GUID DiskType = { 0x77ab535a, 0x45fc, 0x624b, {0x55,0x60,0xf7,0xb2,0x81,0xd1,0xf9,0x6e} };
  if ((Address & 7) || Address < 0xA0000000 || Address > 0xAFFFFFD8) return;
  if (FdtCheckHeader (Dtb) || FdtGetHeader (Dtb, TotalSize) > 0x100000 ||
      FdtGetHeader (Dtb, TotalSize) > 0xB0000000 - Address) return;
  Node = FdtPathOffset (Dtb, "/chosen");
  if (Node < 0) return;
  Start = ReadInitrdProperty (Dtb, Node, "linux,initrd-start");
  End = ReadInitrdProperty (Dtb, Node, "linux,initrd-end");
  Print (L"DXE INITRD start=%lx end=%lx\n", Start, End);
  if (Start < 0xA0000000 || End > 0xB0000000 || End <= Start || End - Start < 512 ||
      End - Start > 0x06000000 || (Start < Address + FdtGetHeader (Dtb, TotalSize) && End > Address)) {
    Print (L"DXE INITRD rejected: size/range/DTB overlap\n"); return;
  }
  Print (L"DXE INITRD bytes=%lu signature=%02x %02x (expected 55 AA)\n", End - Start,
    ((UINT8 *)(UINTN)Start)[510], ((UINT8 *)(UINTN)Start)[511]);
  if (((UINT8 *)(UINTN)Start)[510] != 0x55 || ((UINT8 *)(UINTN)Start)[511] != 0xAA) return;
  Status = gBS->LocateProtocol (&gEfiRamDiskProtocolGuid, NULL, (VOID **)&RamDisk);
  if (EFI_ERROR (Status)) { Print (L"RAMDISK protocol unavailable: %r\n", Status); return; }
  Status = RamDisk->Register (Start, End - Start, &DiskType, NULL, &DiskPath);
  Print (L"RAMDISK Register=%r\n", Status);
  if (!EFI_ERROR (Status)) EfiBootManagerConnectAll ();
}

VOID MarsLinuxProbe (VOID)
{
  VOID *BootHob = GetFirstGuidHob (&gMarsBootDtbHobGuid);
  if (BootHob != NULL && GET_GUID_HOB_DATA_SIZE (BootHob) == sizeof (UINTN)) {
    Print (L"DXE HANDOFF DTB=0x%lx\n", (UINT64)*(UINTN *)GET_GUID_HOB_DATA (BootHob));
    CheckInitrd (*(UINTN *)GET_GUID_HOB_DATA (BootHob));
  }
  EFI_HANDLE *Handles, Kernel;
  UINTN Count, Index, FileIndex;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
  EFI_FILE_PROTOCOL *Root, *File;
  EFI_LOADED_IMAGE_PROTOCOL *Loaded;
  EFI_DEVICE_PATH_PROTOCOL *Path;
  EFI_STATUS Status;
  BOOLEAN Complete;
  STATIC CHAR16 *Files[] = {
    L"\\EFI\\MARSDIAG\\Image",
    L"\\EFI\\MARSDIAG\\mars-efi-minimal.dtb",
    L"\\EFI\\MARSDIAG\\mars-diagnostic.cpio.gz"
  };
  STATIC CHAR16 Options[] = L"dtb=\\EFI\\MARSDIAG\\mars-efi-minimal.dtb initrd=\\EFI\\MARSDIAG\\mars-diagnostic.cpio.gz console=tty0 loglevel=8 ignore_loglevel swiotlb=16384 rdinit=/init maxcpus=8 cpuidle.off=1 clk_ignore_unused pd_ignore_unused panic=0";
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &Count, &Handles);
  if (EFI_ERROR (Status)) { Print (L"Linux probe: no filesystem (%r)\n", Status); return; }
  for (Index = 0; Index < Count; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
    if (EFI_ERROR (Status)) continue;
    Status = Fs->OpenVolume (Fs, &Root);
    if (EFI_ERROR (Status)) continue;
    Complete = TRUE;
    for (FileIndex = 0; FileIndex < 3; FileIndex++) {
      Status = Root->Open (Root, &File, Files[FileIndex], EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR (Status)) { Complete = FALSE; break; }
      File->Close (File);
    }
    Root->Close (Root);
    if (!Complete) continue;
    Path = FileDevicePath (Handles[Index], Files[0]);
    if (Path == NULL) continue;
    Status = gBS->LoadImage (FALSE, gImageHandle, Path, NULL, 0, &Kernel);
    FreePool (Path);
    Print (L"Linux probe: LoadImage=%r\n", Status);
    if (EFI_ERROR (Status)) continue;
    Status = gBS->HandleProtocol (Kernel, &gEfiLoadedImageProtocolGuid, (VOID **)&Loaded);
    if (!EFI_ERROR (Status)) {
      Loaded->LoadOptions = Options;
      Loaded->LoadOptionsSize = sizeof (Options);
      gBS->SetWatchdogTimer (0, 0, 0, NULL);
      Print (L"Starting mars Linux diagnostic\n");
      Status = gBS->StartImage (Kernel, NULL, NULL);
      Print (L"Linux returned: %r\n", Status);
      gBS->Stall (20000000);
    }
    gBS->UnloadImage (Kernel);
  }
  FreePool (Handles);
  Print (L"Linux diagnostic package absent or failed; returning to menu.\n");
}
