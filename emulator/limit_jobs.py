# Pre-build script: limit parallel compile jobs for environments with constrained memory
# (e.g. LOLIN S2 Mini on Windows — cc1plus.exe hits 32-bit virtual address space limit)
Import("env")
env.SetOption("num_jobs", 1)
