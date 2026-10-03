Import("env")
import os

home_packages = os.path.expanduser("~/.platformio/packages")
mingw_bin = os.path.join(home_packages, "toolchain-gccmingw32", "bin")
if os.path.exists(mingw_bin):
    # Update SCons environment PATH
    env.PrependENVPath("PATH", mingw_bin)
    
    # Update OS environment PATH
    os.environ["PATH"] = mingw_bin + os.pathsep + os.environ.get("PATH", "")

    env.Replace(
        CC=os.path.join(mingw_bin, "gcc.exe"),
        CXX=os.path.join(mingw_bin, "g++.exe"),
        AR=os.path.join(mingw_bin, "ar.exe"),
        RANLIB=os.path.join(mingw_bin, "ranlib.exe")
    )
    
    # Statically link libgcc, libstdc++, libwinpthread so the test binary runs without external DLLs
    env.Append(
        LINKFLAGS=[
            "-static",
            "-static-libgcc",
            "-static-libstdc++"
        ]
    )
