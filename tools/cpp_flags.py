Import("env")

print("Adding extra CPP Flags -Wno-deprecated-enum-enum-conversion")

# General options that are passed to the C++ compiler
env.Append(CXXFLAGS=['-Wno-deprecated-enum-enum-conversion'])