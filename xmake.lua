set_policy("check.auto_ignore_flags", false)
add_rules("mode.debug", "mode.release")

set_policy("build.ccache", false)

-- =========================
-- TOOLCHAIN
-- =========================
toolchain("x86_64-elf")
    set_kind("standalone")

    set_toolset("cc", "x86_64-elf-gcc")
    set_toolset("cxx", "x86_64-elf-g++")
    set_toolset("ld", "x86_64-elf-ld")
    set_toolset("sh", "x86_64-elf-gcc")
    set_toolset("as", "nasm")
toolchain_end()

-- =========================
-- PROGRAMS
-- =========================
local user_programs = {"blank", "shell", "echo"}
local kernel_sector_offset = 400
local ovmf_code = "/usr/share/edk2/x64/OVMF_CODE.4m.fd"
local ovmf_vars = "/usr/share/edk2/x64/OVMF_VARS.4m.fd"

includes("programs/stdlib")
includes("programs/blank")
includes("programs/shell")
includes("programs/echo")

-- =========================
-- BOOTLOADER
-- =========================
target("boot")
    set_kind("phony")
    set_default(false)
    add_deps("kernel")

    on_build(function ()
        os.exec("mkdir -p bin")

        local kernel_size = os.filesize("bin/kernel.bin")
        if not kernel_size then
            raise("bin/kernel.bin not found; build kernel first")
        end

        local kernel_sectors = math.ceil(kernel_size / 512)
        if kernel_sectors > 0x1000 then
            raise(string.format("kernel too large (%d sectors)", kernel_sectors))
        end

        os.exec(string.format(
            "nasm -f bin -DKERNEL_LBA_START=%d -DKERNEL_SECTOR_COUNT=%d src/bootloader/boot.asm -o bin/boot.bin",
            kernel_sector_offset,
            kernel_sectors
        ))
    end)
target_end()

-- =========================
-- KERNEL ASM
-- =========================
target("kernel_asm")
    set_kind("object")
    set_toolchains("x86_64-elf")

    set_toolset("as", "nasm")
    set_objectdir("bin/obj/kernel_asm")
    set_symbols("debug")

    add_files(
        "src/kernel.asm",
        "src/idt/idt.asm",
        "src/io/io.asm",
        "src/memory/paging/paging.asm",
        "src/gdt/gdt.asm",
        "src/task/tss.asm",
        "src/task/task.asm"
    )

    add_asflags("-f elf64", "-g", "-F dwarf")
target_end()

-- =========================
-- KERNEL C
-- =========================
target("kernel_c")
    set_kind("object")
    set_toolchains("x86_64-elf")
    set_languages("gnu99")

    add_includedirs("include", {public = true})
    add_cflags("-Iinclude", "-nostdinc")
    
    set_objectdir("bin/obj/kernel_c")
    set_symbols("debug")

    add_files("src/*.c", "src/**/*.c")

    add_cflags(
        "-ffreestanding",
        "-nostdinc",
        "-Iinclude",
        "-g",
        "-O0",
        "-fno-builtin",
        "-m64",
        "-fno-pic",
        "-fno-pie",
        "-fno-stack-protector"
    )
target_end()

-- =========================
-- KERNEL LINK
-- =========================
target("kernel")
    set_kind("binary")
    set_toolchains("x86_64-elf")

    set_symbols("debug")
    set_targetdir("bin")
    set_filename("kernel.bin")

    add_deps("kernel_asm", "kernel_c")

    add_ldflags(
        "-m", "elf_x86_64",
        "-T", "src/linker.ld",
        "-nostdlib",
        "--no-pie"
    )
target_end()

-- =========================
-- ASSETS
-- =========================
target("assets")
    set_kind("phony")
    set_default(false)
    add_deps("libvios", "blank", "shell", "echo")

    on_build(function ()
        os.rm("bin/assets")
        os.exec("mkdir -p bin/assets")

        for _, program in ipairs(user_programs) do
            local src = "assets/" .. program .. ".elf"
            local dst = "bin/assets/" .. program  -- remove .elf
            os.cp(src, dst)
        end
    end)
target_end()

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
        os.exec("mformat -i bin/os.bin -t 1024 -h 1 -n 64 -B bin/boot.bin ::")
        os.exec(string.format(
            "dd if=bin/kernel.bin of=bin/os.bin bs=512 seek=%d conv=notrunc status=none",
            kernel_sector_offset
        ))

        for _, program in ipairs(user_programs) do
            os.exec("mcopy -o -i bin/os.bin bin/assets/" .. program .. " ::")
        end
    end)
target_end()

-- =========================
-- QEMU
-- =========================
target("run-bios")
    set_kind("phony")
    set_default(false)
    add_deps("image")

    on_build(function ()
        os.exec("qemu-system-x86_64 -drive file=bin/os.bin,format=raw,if=ide")
    end)
target_end()

target("run-edk2")
    set_kind("phony")
    set_default(false)
    add_deps("image")

    on_build(function ()
        if not os.isfile(ovmf_code) or not os.isfile(ovmf_vars) then
            raise("EDK2 OVMF firmware not found. Expected " .. ovmf_code .. " and " .. ovmf_vars)
        end

        os.cp(ovmf_vars, "bin/OVMF_VARS.fd")
        os.exec(string.format(
            "qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=%s -drive if=pflash,format=raw,file=bin/OVMF_VARS.fd -drive file=bin/os.bin,format=raw,if=ide",
            ovmf_code
        ))
    end)
target_end()
