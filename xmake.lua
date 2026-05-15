-- =========================================================
-- POLICIES
-- =========================================================
set_policy("check.auto_ignore_flags", false)
set_policy("build.ccache", false)

add_rules("mode.debug", "mode.release")

-- =========================================================
-- CONFIG
-- =========================================================
local is_macos = os.host() == "macosx"

local docker_image = "vios-build"

local user_programs = {
    "blank",
    "shell",
    "echo"
}

local kernel_sector_offset = 400
local tempdir = "bin/tmp"

local ovmf_code = "/usr/share/edk2/x64/OVMF_CODE.4m.fd"
local ovmf_vars = "/usr/share/edk2/x64/OVMF_VARS.4m.fd"

local edk2_efi_output = "src/bootloader/edk2/Build/ViOS/RELEASE_GCC/X64/ViOSBootloader.efi"

local function latest_mtime(paths)
    local latest = 0
    for _, path in ipairs(paths) do
        local mtime = os.mtime(path)
        if mtime and mtime > latest then
            latest = mtime
        end
    end
    return latest
end

local function should_rebuild_edk2()
    if not os.isfile(edk2_efi_output) then
        return true
    end

    local edk2_sources = os.files(
        "src/bootloader/edk2/build-edk2.sh",
        "src/bootloader/edk2/ViOSPkg/**.c",
        "src/bootloader/edk2/ViOSPkg/**.h",
        "src/bootloader/edk2/ViOSPkg/**.inf",
        "src/bootloader/edk2/ViOSPkg/**.dsc",
        "src/bootloader/edk2/ViOSPkg/**.dec"
    )

    local output_mtime = os.mtime(edk2_efi_output) or 0
    return latest_mtime(edk2_sources) > output_mtime
end

-- =========================================================
-- TOOLCHAIN
-- =========================================================
toolchain("vios")
    set_kind("standalone")
    set_toolset("cc", "gcc")
    set_toolset("cxx", "g++")
    set_toolset("ld", "ld")
    set_toolset("sh", "gcc")
    set_toolset("as", "nasm")
toolchain_end()

-- =========================================================
-- PROGRAMS
-- =========================================================
includes("programs/stdlib")
includes("programs/blank")
includes("programs/shell")
includes("programs/echo")

-- =========================================================
-- DOCKER (ONLY ON MACOS)
-- =========================================================
if is_macos then
    task("docker-build")
        set_menu {
            usage = "xmake docker-build",
            description = "Build Docker image"
        }
        on_run(function ()
            os.exec("docker build -t %s .", docker_image)
        end)
    task_end()

    task("docker")
        set_menu {
            usage = "xmake docker",
            description = "Build project in Docker"
        }
        on_run(function ()
            local cmd = string.format(
                'docker run --rm ' ..
                '-e XMAKE_ROOT=y ' ..
                '-v "%s:/workspace" ' ..
                '-w /workspace ' ..
                '%s sh -c "xmake f -c && xmake"',
                os.projectdir(),
                docker_image
            )
            os.exec(cmd)
        end)
    task_end()

    task("docker-run")
        set_menu {
            usage = "xmake docker-run",
            description = "Run project in Docker"
        }
        on_run(function ()
            local cmd = string.format(
                'docker run --rm -it ' ..
                '-e XMAKE_ROOT=y ' ..
                '-v "%s:/workspace" ' ..
                '-w /workspace ' ..
                '%s sh -c "xmake b run-bios"',
                os.projectdir(),
                docker_image
            )
            os.exec(cmd)
        end)
    task_end()
end

-- =========================================================
-- BOOTLOADER
-- =========================================================
target("boot")
    set_kind("phony")
    set_default(false)
    add_deps("kernel")

    on_build(function ()
        os.mkdir("bin")
        local kernel_size = os.filesize("bin/kernel.bin")
        if not kernel_size then
            raise("bin/kernel.bin not found")
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

-- =========================================================
-- KERNEL ASM
-- =========================================================
target("kernel_asm")
    set_kind("object")
    set_toolchains("vios")
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

-- =========================================================
-- KERNEL C
-- =========================================================
target("kernel_c")
    set_kind("object")
    set_toolchains("vios")
    set_languages("gnu99")
    set_objectdir("bin/obj/kernel_c")
    set_symbols("debug")
    add_includedirs("include", {public = true})
    add_files("src/*.c", "src/**/*.c")
    remove_files("src/bootloader/edk2/**/*.c")
    on_load(function ()
        local tempdir_abs = os.projectdir() .. "/" .. tempdir
        os.mkdir(tempdir_abs)
        os.addenvs({
            TMPDIR = tempdir_abs,
            TMP = tempdir_abs,
            TEMP = tempdir_abs
        })
    end)
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

-- =========================================================
-- KERNEL LINK
-- =========================================================
target("kernel")
    set_kind("binary")
    set_toolchains("vios")
    set_targetdir("bin")
    set_filename("kernel.bin")
    set_symbols("debug")
    add_deps("kernel_asm", "kernel_c")
    add_ldflags("-m", "elf_x86_64", "-T", "src/linker.ld", "-nostdlib", "--no-pie")
target_end()

-- =========================================================
-- UEFI IMAGE
-- =========================================================
target("uefi")
    set_kind("phony")
    set_default(true)
    add_deps("kernel", "assets")
    on_build(function ()
        if should_rebuild_edk2() then
            cprint("${yellow}[uefi] Building EDK2 bootloader...${clear}")
            os.execv("sh", {"-c", string.format("cd %s/src/bootloader/edk2 && ./build-edk2.sh", os.projectdir())})
        else
            cprint("${green}[uefi] EDK2 bootloader is up to date.${clear}")
        end

        os.mkdir("bin/uefi/EFI/BOOT")
        cprint("${yellow}[uefi] Creating bin/os-uefi.img...${clear}")
        os.exec("dd if=/dev/zero of=bin/os-uefi.img bs=1M count=16 status=none")
        os.exec("mformat -i bin/os-uefi.img -t 1024 -h 1 -n 64 ::")
        os.exec("mmd -i bin/os-uefi.img ::/EFI")
        os.exec("mmd -i bin/os-uefi.img ::/EFI/BOOT")
        os.exec("mmd -i bin/os-uefi.img ::/bin")
        os.cp(edk2_efi_output, "bin/uefi/EFI/BOOT/BOOTX64.EFI")
        os.exec("mcopy -o -i bin/os-uefi.img bin/uefi/EFI/BOOT/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI")
        os.exec("mcopy -o -i bin/os-uefi.img bin/kernel.bin ::/kernel.bin")
        -- copy all top-level asset subdirectories (bin, images, fonts, etc.) into image root
        os.execv("sh", {"-c", "mcopy -o -i bin/os-uefi.img -s assets/* ::/"})
    end)
target_end()

-- =========================================================
-- BIOS COMMAND
-- =========================================================
task("bios")
    set_menu {
        usage = "xmake bios",
        description = "Build the legacy BIOS image"
    }
    on_run(function ()
        os.execv("sh", {"-c", string.format("cd %s && xmake b bios-image", os.projectdir())})
    end)
task_end()

-- =========================================================
-- ASSETS
-- =========================================================
target("assets")
    set_kind("phony")
    set_default(false)
    add_deps("libvios", "blank", "shell", "echo")
    on_build(function ()
        os.mkdir("assets/bin")
    end)
target_end()

-- =========================================================
-- BIOS IMAGE
-- =========================================================
target("bios-image")
    set_kind("phony")
    set_default(false)
    add_deps("boot", "kernel", "assets")
    on_build(function ()
        os.mkdir("bin")
        os.exec("dd if=/dev/zero of=bin/os.bin bs=1M count=16 status=none")
        os.exec("mformat -i bin/os.bin -t 1024 -h 1 -n 64 -B bin/boot.bin ::")
        os.exec("mmd -i bin/os.bin ::/bin")
        os.exec(string.format(
            "dd if=bin/kernel.bin of=bin/os.bin bs=512 seek=%d conv=notrunc status=none",
            kernel_sector_offset
        ))
        -- copy all top-level asset subdirectories into BIOS image root as well
        os.execv("sh", {"-c", "mcopy -o -i bin/os.bin -s assets/* ::/"})
    end)
    after_build(function (target)
        if is_macos then
            os.exec("sudo chown -R $(whoami) bin")
        end
    end)
target_end()
