[Defines]
  PLATFORM_NAME                  = ViOS
  PLATFORM_GUID                  = 87654321-4321-4321-4321-210987654321
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ViOS
  SUPPORTED_ARCHITECTURES        = X64

  SKUID_IDENTIFIER               = DEFAULT

[LibraryClasses]
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  PrintLib|MdePkg/Library/PrintLib/PrintLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf

[Components]
  ViOSPkg/Application/ViOSBootloader/ViOSBootloader.inf
