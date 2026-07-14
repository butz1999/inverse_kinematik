from os.path import isfile, join

Import("env")

platform = env.PioPlatform()
gdb_package_dir = platform.get_package_dir("tool-xtensa-esp-elf-gdb")

if gdb_package_dir:
    gdb_path = join(gdb_package_dir, "bin", "xtensa-esp32s3-elf-gdb")
    if isfile(gdb_path):
        env.Replace(GDB=gdb_path)
        env.PrependENVPath("PATH", join(gdb_package_dir, "bin"))
