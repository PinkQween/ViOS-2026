set_policy("check.auto_ignore_flags", false)
add_rules("mode.debug", "mode.release")

-- =========================
-- TOOLCHAIN
-- =========================
toolchain("i686-elf")
    set_kind("standalone")

    set_toolset("cc", "i686-elf-gcc")
    set_toolset("cxx", "i686-elf-g++")
    set_toolset("ld", "i686-elf-ld")
    set_toolset("sh", "i686-elf-gcc")
    set_toolset("as", "nasm")
toolchain_end()

-- =========================
-- PROGRAMS
-- =========================
includes("programs/stdlib")
includes("programs/blank")
includes("programs/hello")

-- =========================
-- BOOTLOADER (Legacy BIOS)
-- =========================
target("boot")
    set_kind("phony")
    set_default(false)
    add_deps("kernel")

    on_build(function ()
        os.exec("mkdir -p bin")

        local kernel_size = os.filesize("bin/kernel.bin")
        if not kernel_size then
            raise("bin/kernel.bin not found; build kernel before boot")
        end

        local kernel_sectors = math.ceil(kernel_size / 512)
        if kernel_sectors > 0x1000 then
            raise(string.format("kernel.bin too large for current bootloader (%d sectors > 0x1000)", kernel_sectors))
        end

        os.exec(string.format(
            "nasm -f bin -DKERNEL_SECTOR_COUNT=%d src/bootloader/boot.asm -o bin/boot.bin",
            kernel_sectors
        ))
    end)


-- =========================
-- KERNEL ASM
-- =========================
target("kernel_asm")
    set_kind("object")
    set_toolchains("i686-elf")

    set_toolset("as", "nasm")
    set_objectdir("bin/obj/kernel_asm")
    set_symbols("debug")
    add_files("src/kernel.asm",
              "src/idt/idt.asm",
              "src/io/io.asm",
              "src/memory/paging/paging.asm",
              "src/gdt/gdt.asm",
              "src/task/tss.asm",
              "src/task/task.asm"
    )
    add_asflags("-f elf32")
    add_asflags("-g", "-F dwarf")


-- =========================
-- KERNEL C
-- =========================
target("kernel_c")
    set_kind("object")
    set_toolchains("i686-elf")

    add_includedirs("include")
    set_objectdir("bin/obj/kernel_c")
    set_symbols("debug")
    add_files("src/kernel.c", "src/**/*.c")
    add_cflags(
        "-ffreestanding",
        "-g",
        "-O0",
        "-fno-builtin",
        "-m32",
        "-fno-pic",
        "-fno-pie"
    )


-- =========================
-- KERNEL LINK
-- =========================
target("kernel")
    set_kind("binary")
    set_toolchains("i686-elf")
    set_symbols("debug")
    set_targetdir("bin")
    set_filename("kernel.bin")
    set_toolset("ld", "i686-elf-ld")

    add_deps("kernel_asm", "kernel_c")

    add_ldflags(
        "-m", "elf_i386",
        "-T", "src/linker.ld",
        "-nostdlib",
        "--no-pie"
    )

-- =========================
-- ASSETS
-- =========================
target("assets")
    set_kind("phony")
    set_default(false)
    add_deps("vios", "hello", "blank")

    on_build(function ()
        os.exec("mkdir -p bin/assets")
        
        -- Copy built programs to bin/assets directory
        os.cp("assets/*.elf", "bin/assets/")
        os.cp("assets/*.a", "bin/assets/")
    end)

-- =========================
-- IMAGE
-- =========================
target("image")
    set_kind("phony")
    set_default(true)
    add_deps("boot", "kernel", "assets")

    on_build(function ()
        os.exec("mkdir -p bin")

        os.exec("dd if=/dev/zero of=bin/os.bin bs=1M count=16 status=none")
        os.exec("mformat -i bin/os.bin -t 1024 -h 1 -n 32 -B bin/boot.bin ::")
        os.exec("dd if=bin/kernel.bin of=bin/os.bin bs=512 seek=400 conv=notrunc status=none")

        if os.isdir("assets") then
            os.exec('sh -c "ls assets/* >/dev/null 2>&1 && mcopy -o -i bin/os.bin -s assets/* :: || true"')
        end
    end)
