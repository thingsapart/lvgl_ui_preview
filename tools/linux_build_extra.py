Import("env", "projenv")

# Override unused "upload" to execute compiled binary
from SCons.Script import AlwaysBuild
AlwaysBuild(env.Alias("build", "$BUILD_DIR/${PROGNAME}", "$BUILD_DIR/${PROGNAME}"))

# Add custom target to explorer
env.AddTarget(
    name = "execute",
    dependencies = "$BUILD_DIR/${PROGNAME}",
    actions = "$BUILD_DIR/${PROGNAME}",
#    actions = 'cmd.exe /C "start cmd.exe /C $BUILD_DIR\${PROGNAME}.exe"',
    title = "Execute",
    description = "Build and execute",
    group="General"
)

#print('=====================================')
#print(env.Dump())