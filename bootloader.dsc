[Defines]
    DSC_SPECIFICATION               = 0x0001001C
    PLATFORM_NAME                   = Bootloader
    PLATFORM_GUID                   = 68e26d69-87fa-4fe3-a0ab-d745d6b9a8c3
    PLATFORM_VERSION                = 1.0
    SKUID_IDENTIFIER                = DEFAULT
    SUPPORTED_ARCHITECTURES         = X64
    BUILD_TARGETS                   = DEBUG | RELEASE

[LibraryClasses]
    UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
    PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
    BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
    PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
    MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
    UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
    StackCheckLib|MdePkg/Library/StackCheckLib/StackCheckLib.inf
    StackCheckFailureHookLib|MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf

[Components]
    bootloader.inf