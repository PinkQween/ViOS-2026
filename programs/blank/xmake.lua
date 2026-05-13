target("blank")
    set_kind("phony")
    set_default(false)

    add_deps("libvios")

    on_build(function ()
        os.mkdir("programs/blank/build")
        os.mkdir("assets/bin")

        local cflags =
            "-c " ..
            "-std=gnu99 " ..
            "-ffreestanding " ..
            "-nostdinc " ..
            "-nostdlib " ..
            "-fno-builtin " ..
            "-m64 " ..
            "-fno-pic " ..
            "-fno-pie " ..
            "-fno-stack-protector " ..
            "-g " ..
            "-O0 " ..
            "-Iprograms/stdlib/include "

        os.exec(
            "gcc " .. cflags ..
            "programs/blank/src/main.c " ..
            "-o programs/blank/build/blank.o"
        )

        os.exec(
            "ld -m elf_x86_64 " ..
            "-T programs/blank/linker.ld " ..
            "-nostdlib " ..
            "-o assets/bin/blank " ..
            "programs/stdlib/build/start.o " ..
            "programs/blank/build/blank.o " ..
            "programs/stdlib/build/libvios.a"
        )
    end)
target_end()