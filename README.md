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

## Boot With Qemu (MacOs specific)

Head into the demo directory.

Create an empty qcow image
```shell
qemu-img create -f qcow2 disk.img 10G
```
Paste the following command which launches qemu (with 4gb ram, 4 cpu cores).
The `boot_disk.img` is the filesystem where the bootloader elf lives. The bootloader
now starts and boots tinycore.
```shell
qemu-system-x86_64 \
  -machine q35 \
  -m 4G \
  -smp 4 \
  -drive file=disk.img,format=qcow2 \
  -boot menu=on \
  -drive file=boot_disk.img,format=raw \
  -drive if=pflash,format=raw,readonly=on,file=/opt/homebrew/share/qemu/edk2-x86_64-code.fd \
  -no-reboot -no-shutdown -serial stdio -d int,cpu_reset,guest_errors
```

## Test your own changes (MacOs specific)

Head into the demo directory.

Create an empty qcow image
```shell
qemu-img create -f qcow2 disk.img 10G
```
Set BOOT_HOME env variable to the current directory
```shell
export BOOT_HOME={your current directory (absolute path)}
```
Then copy (delete old one first) your bootloader elf into the share directory which
provides the connection between qemu and host system.
```shell
rm ./$BOOT_HOME/BOOTX64.EFI
mv ./your_bootloader.elf $BOOT_HOME/share/
```
Then execute the following command which launches qemu (4gb ram, 4 cpu cores).
`boot_disk.img` is the filesystem where the bootloader elf later gets pasted.
```shell
qemu-system-x86_64 \
  -machine q35 \
  -m 4G \
  -smp 4 \
  -drive file=disk.img,format=qcow2 \
  -boot menu=on \
-drive file=boot_disk.img,format=raw \
-drive file=fat:rw:$BOOT_HOME/share,format=raw  
```
Now tinycore should start. Go into tinycore's terminal and paste
```shell
sudo mount /dev/sdb1 /mnt/sdb1
sudo mount /dev/sdc1 /mnt/sdc1
sudo mv /mnt/sdc1/B00TX64.EFI /mnt/sdb1/EFI/BOOT/
umount /mnt/sdc1
umount /mnt/sdb1
```
This moves the bootloader elf from the share directory into the `boot_disk.img`.

Now you can test your changes ths same way as mentioned above - (Section "Boot With Qemu (MacOs specific)").
