from os.path import join
from os.path import expandvars
Import("env")

print("Current CLI targets", COMMAND_LINE_TARGETS)
print("Current Build targets", BUILD_TARGETS)

# Ugly hack to remove some source files. build_src_filter doesn't seem to work
# on libdeps, so we just overwrite the source files in question with Noop source.        
def empty_src_file(source, target, env):
    print("$!!", source[0].get_abspath(), target[0].get_abspath())
    with open(source[0].get_abspath(), 'w') as f:
        f.write("#define __EMPTIED__\n")

env.AddPostAction( ".pio/build/darwin_sdl/lib51d/cJSON/fuzzing/fuzz_main.o", empty_src_file)
env.AddPostAction( ".pio/build/darwin_sdl/lib51d/cJSON/fuzzing/afl.o", empty_src_file)
env.AddPostAction( ".pio/build/darwin_sdl/lib51d/cJSON/test.o", empty_src_file)


def post_fuzz(source, target, env):
    print(source[0].get_abspath(), target[0].get_abspath())
    env.Execute("rm " + target[0].get_abspath())

# env.AddPostAction(
#    ".pio/build/darwin_sdl/lib51d/cJSON/fuzzing/fuzz_main.o",
#    post_fuzz)