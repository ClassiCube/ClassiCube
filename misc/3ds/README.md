To see debug log messages in Citra:

1) Make sure log level set to "Debug.Emulated:Debug"

---

Commands used to generate the .bin files:

`bannertool makebanner -i banner.png -a audio.wav -o banner.bin`

`bannertool makesmdh -s ClassiCube -l ClassiCube -p UnknownShadow200 -i icon.png -o icon.bin`

----

Debug log messages output to debug service, so may be possible to see from console via remote gdb
