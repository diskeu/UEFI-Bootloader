# 64-BIT UEFI Bootloader
Tiny Bootloader (implementing 64-Bit Linux Boot Protocol) that can
boot from UEFI supportable systems with an x86_64 architecture.

Tested using QEMU emulation on an ARM64-based host system, booting TinyLinux.

## Process
The bootloader starts by initializing the required UEFI protocols (e.g. SimpleFileSystemProtocol)
to be able to read configuration file specified by the BOOT_CONFIG_PATH macro (default: `\boot\config`).
It then parses the configuration file, which contains key-value pairs following a simplified format inspired by [Linux Boot Configuration](https://docs.kernel.org/admin-guide/bootconfig.html).
Next, it searches for the kernel path using the key defined by the KERNEL_PATH_KEY macro (default: `kernel`).
The corresponding value specifies the location of the kernel image.
Then, the bootloader sets up the necessary fields in the `boot_params` including the `setup_header`, `cmd_line_ptr`, `loadflags` etc.
It then constructs the `E820 memory map` (e820_table) by iterating over the UEFI memory descriptors and converting/translating the memory types
into corresponding E820 memory types.
Finally, the bootloader exits UEFI Boot Services and executes the final assembly snippet that disables interrupts, clears required registers,
and finally jumps to the kernel entry point.

### Note
The Bootloader currently freezes after jumping to the kernel entry point. I couldn't identify the cause of this issue yet.
