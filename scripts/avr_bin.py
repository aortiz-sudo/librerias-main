# Genera la imagen cruda (.bin) del firmware de un ATmega al terminar de enlazar.
#
# PlatformIO solo produce firmware.elf y firmware.hex para atmelavr, pero el
# bootloader I2C recibe el programa página por página en binario crudo, desde la
# dirección 0. El nombre del archivo sale de `custom_fw_bin_name` en platformio.ini
# (display -> display.bin, printer -> printer.bin), que es la ruta que descarga el
# ESP32 desde firmware_repository.

Import("env")

name = env.GetProjectOption("custom_fw_bin_name")

env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    env.VerboseAction(
        '"$OBJCOPY" -O binary -R .eeprom "$BUILD_DIR/${PROGNAME}.elf" "$BUILD_DIR/%s.bin"' % name,
        "Generando $BUILD_DIR/%s.bin" % name,
    ),
)
