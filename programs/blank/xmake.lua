target("blank")
    set_kind("binary")
    set_toolchains("i686-elf")
    set_targetdir("../../assets")
    set_filename("blank.elf")
    set_toolset("ld", "i686-elf-ld")
    set_toolset("as", "nasm")
    
    after_load(function (target)
        target:add("deps", "vios", {inherit = false})
    end)
    
    add_files("blank.c")
    add_files("../stdlib/src/start.asm")
    add_includedirs("../stdlib/include")
    
    add_cflags("-m32", "-ffreestanding", "-fno-builtin", "-fno-pic", "-fno-pie", "-g")
    add_asflags("-f", "elf32")
    add_ldflags("-m", "elf_i386", "-T", "$(projectdir)/programs/blank/linker.ld", "-nostdlib", "$(projectdir)/assets/libvios.a")
