# Corrige la dirección donde PlatformIO escribe boot_app0.bin al cargar por USB.
#
# El framework de Arduino-ESP32 lo manda siempre a 0xe000 (donde está otadata en la tabla de
# particiones por defecto), pero partitions.csv de este proyecto tiene otadata en 0x9000. Sin
# esto, una carga por USB no reinicia otadata: después de una OTA que dejó el arranque en app1,
# el ESP32 seguiría arrancando el firmware de app1 aunque se cargara uno nuevo en app0.
#
# Se aplica después del builder de la plataforma (post:), que ya agregó las imágenes extra a
# los flags de esptool (platforms/espressif32/builder/main.py).

Import("env")

import csv


def otadata_offset(partitions_csv):
    with open(partitions_csv, newline="") as f:
        for row in csv.reader(f):
            fields = [c.strip() for c in row]
            if not fields or fields[0].startswith("#") or len(fields) < 4:
                continue
            if fields[1] == "data" and fields[2] == "ota":
                return fields[3]
    return None


partitions_csv = env.subst("$PARTITIONS_TABLE_CSV")
offset = otadata_offset(partitions_csv)

if offset is None:
    print("ADVERTENCIA: no hay particion otadata en %s; boot_app0.bin queda en 0xe000" % partitions_csv)
else:
    env.Replace(FLASH_EXTRA_IMAGES=[
        (offset if str(image).endswith("boot_app0.bin") else address, image)
        for address, image in env.get("FLASH_EXTRA_IMAGES", [])
    ])

    flags = list(env.get("UPLOADERFLAGS", []))
    for i in range(1, len(flags)):
        if str(flags[i]).endswith("boot_app0.bin"):
            flags[i - 1] = offset
    env.Replace(UPLOADERFLAGS=flags)
