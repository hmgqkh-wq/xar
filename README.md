Xclipse 940 Vulkan Forwarding ICD Wrapper (ARM64)

This repository contains only source files and text files.
There are NO executable files, NO precompiled binaries.

Files include:
- manifest.json
- CMakeLists.txt
- src/*.c
- include/*.h
- .github/workflows/build.yml

Use GitHub Actions to compile the wrapper into libxeno_wrapper.so.
Then import the generated ZIP into Winlator.

Logs and feature dumps will be written to:
    /storage/emulated/0/xclipse_output/
    /sdcard/xclipse_output/

Upload the generated xeno_tune_report.json and xeno_wrapper.log
to ChatGPT for full tuning of your Xclipse 940 GPU.
