target("libvios")
    set_kind("phony")
    set_default(false)

    on_build(function ()
        os.mkdir("programs/stdlib/build")
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
            "nasm -f elf32 " ..
            "programs/stdlib/src/start.asm " ..
            "-o programs/stdlib/build/start.o"
        )

        os.exec(
            "nasm -f elf32 " ..
            "programs/stdlib/src/vios.asm " ..
            "-o programs/stdlib/build/vios.asm.o"
        )

        os.exec(
            "gcc " .. cflags ..
            "programs/stdlib/src/start.c " ..
            "-o programs/stdlib/build/start.c.o"
        )

        os.exec(
            "gcc " .. cflags ..
            "programs/stdlib/src/vios.c " ..
            "-o programs/stdlib/build/vios.o"
        )

        os.exec(
            "gcc " .. cflags ..
            "programs/stdlib/src/stdlib.c " ..
            "-o programs/stdlib/build/stdlib.o"
        )

        os.exec(
            "gcc " .. cflags ..
            "programs/stdlib/src/stdio.c " ..
            "-o programs/stdlib/build/stdio.o"
        )

        os.exec(
            "gcc " .. cflags ..
            "programs/stdlib/src/string.c " ..
            "-o programs/stdlib/build/string.o"
        )
    end)
target_end()