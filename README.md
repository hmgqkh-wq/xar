This repository builds the Xclipse 940 Vulkan wrapper (balanced mode).

After building on GitHub Actions, download the artifact ZIP.
Inside you will find:

- libxeno_wrapper.so
- xclipse_feature_dump_generator (test binary)
- logs (if any generated)
- manifest.json (copied)
- feature_dump.txt (if generator produces one)

To continue tuning:
1. Take libxeno_wrapper.so and place inside Winlator driver folder.
2. Run any Vulkan app in Winlator.
3. Two files will appear on device:
   /sdcard/xclipse/xeno_wrapper.log
   /sdcard/xclipse/xeno_tune_report.json

Upload both files to ChatGPT.
