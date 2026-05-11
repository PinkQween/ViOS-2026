target("echo")
    set_kind("phony")
    set_default(false)

    add_deps("libvios")

    on_build(function ()
        os.mkdir("programs/echo/build")
        os.mkdir("assets")

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
            "programs/echo/src/main.c " ..
            "-o programs/echo/build/echo.o"
        )

        os.exec(
            "ld -m elf_x86_64 " ..
            "-T programs/echo/linker.ld " ..
            "-nostdlib " ..
            "-o assets/echo.elf " ..
            "programs/stdlib/build/start.o " ..
            "programs/echo/build/echo.o " ..
            "assets/libvios.a"
        )
    end)
target_end()