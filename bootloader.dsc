[Defines]
    DSC_SPECIFICATION               = 0x0001001C
    PLATFORM_NAME                   = Bootloader
    PLATFORM_GUID                   = 68e26d69-87fa-4fe3-a0ab-d745d6b9a8c3
    PLATFORM_VERSION                = 1.0
    SKUID_IDENTIFIER                = DEFAULT
    SUPPORTED_ARCHITECTURES         = X64 | AARCH64
    BUILD_TARGETS                   = DEBUG | RELEASE

[LibraryClasses]
    UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf

[Components]
    bootloader.inf