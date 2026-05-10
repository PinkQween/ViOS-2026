target("shell")
    set_kind("phony")
    set_default(false)

    add_deps("libvios")

    on_build(function ()
        os.mkdir("programs/shell/build")
        os.mkdir("assets")

        local cflags =
            "-c " ..
            "-std=gnu99 " ..
            "-ffreestanding " ..
            "-nostdinc " ..
            "-nostdlib " ..
            "-fno-builtin " ..
            "-m32 " ..
            "-fno-pic " ..
            "-fno-pie " ..
            "-fno-stack-protector " ..
            "-g " ..
            "-O0 " ..
            "-Iprograms/stdlib/include "

        os.exec(
            "gcc " .. cflags ..
            "programs/shell/src/main.c " ..
            "-o programs/shell/build/shell.o"
        )

        os.exec(
            "ld -m elf_i386 " ..
            "-T programs/shell/linker.ld " ..
            "-nostdlib " ..
            "-o assets/shell.elf " ..
            "programs/stdlib/build/start.o " ..
            "programs/shell/build/shell.o " ..
            "assets/libvios.a"
        )
    end)
target_end()