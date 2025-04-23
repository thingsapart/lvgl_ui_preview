Import("env")


#env.AddCustomTarget("my_test", None, 'python C:\Users\fvanroie\.platformio\packages\toolchain-xtensa32\bin\elf-size-analyze.py -a -H -w 120 $BUILD_DIR/${PROGNAME} -F -m 250')

# Add custom target to explorer
env.AddTarget(
    name = "analyze_ram",
    dependencies = "$BUILD_DIR/${PROGNAME}${PROGSUFFIX}",
    actions = 'python $PROJECT_DIR/tools/elf-size-analyze.py $BUILD_DIR/${PROGNAME}${PROGSUFFIX} -t $PROJECT_PACKAGES_DIR\\toolchain-xtensa32\\bin\\xtensa-esp32-elf- -a -H -w 120 -R -m 512',
    title = "Analyze RAM",
    description = "Build and analyze",
    group="Analyze"
)
env.AddTarget(
    name = "analyze_flash",
    dependencies = "$BUILD_DIR/${PROGNAME}${PROGSUFFIX}",
    actions = 'python $PROJECT_DIR/tools/elf-size-analyze.py $BUILD_DIR/${PROGNAME}${PROGSUFFIX} -t $PROJECT_PACKAGES_DIR\\toolchain-xtensa32\\bin\\xtensa-esp32-elf- -a -H -w 120 -F -m 512',
    title = "Analyze Flash",
    description = "Build and analyze",
    group="Analyze"
)

print('Adding CPP flags: -Wno-deprecated-enum-enum-conversion') 
env.Append(CXXFLAGS=['-Wno-deprecated-enum-enum-conversion'])
env.Append(CPPFLAGS=['-Wno-deprecated-enum-enum-conversion'])

build_flags = env['BUILD_FLAGS']
mcu = env.get("BOARD_MCU").lower()

# General options that are passed to the C++ compiler
env.Append(CXXFLAGS=["-Wno-volatile"])
env.Append(CPPFLAGS=["-Wno-volatile"])

# General options that are passed to the C compiler (C only; not C++).
env.Append(CFLAGS=["-Wno-discarded-qualifiers", "-Wno-implicit-function-declaration", "-Wno-incompatible-pointer-types"])

# Remove build flags which are not valid for risc-v
if mcu in ("esp32c2", "esp32c3", "esp32c6", "esp32h2", "esp32p4"):
  try:
    build_flags.pop(build_flags.index("-mno-target-align"))
  except:
    pass
  try:
    build_flags.pop(build_flags.index("-mtarget-align"))
  except:
    pass

print('=====================================>')
