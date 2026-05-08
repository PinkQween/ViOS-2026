toolchain("i686-elf")
    set_kind("standalone")
    set_bindir("/usr/bin")
    set_sdkdir("/usr")
    set_toolset("cc", "i686-elf-gcc")
    set_toolset("as", "nasm")
    set_toolset("ld", "i686-elf-gcc")
    set_toolset("sh", "i686-elf-gcc")
toolchain_end()

target("vios")
    set_kind("static")
    set_toolchains("i686-elf")
    set_targetdir("../../assets")
    set_filename("libvios.a")
    set_toolset("as", "nasm")

    add_files("src/vios.asm", "src/stdlib.c")
    add_includedirs("src", "include")

    set_policy("check.auto_ignore_flags", false)

    add_cflags("-m32", "-ffreestanding", "-fno-builtin", "-g")
    add_asflags("-f", "elf32")
