# extra_script.py
import os
import sys
import tempfile
import subprocess
import shutil
import platform as py_platform
from SCons.Script import DefaultEnvironment, Glob

# Get the PlatformIO environment
env = DefaultEnvironment()
platform = env.PioPlatform()

# --- Configuration ---

# 1. List of Template Header Files to prepend
#    These are the files that will be placed *before* each .def.h content
#    Adjust these paths according to your project structure.
TEMPLATE_HEADERS = [
    os.path.join(env.subst("$PROJECT_SRC_DIR"), "ui", "layout", "lv_view_def.h"),
    os.path.join(env.subst("$PROJECT_SRC_DIR"), "ui", "layout", "lv_view_impl.h"),
    # Add more template headers here if needed
]

# 2. Directory containing the input view definition files (*.def.h)
DEFINITIONS_DIR = os.path.join(env.subst("$PROJECT_SRC_DIR"), "ui", "components")

# 3. Directory for the generated output files
GENERATED_VIEWS_DIR = os.path.join(env.subst("$PROJECT_SRC_DIR"), "ui", "gen_views")

# 4. Glob pattern for the input definition files
DEFINITIONS_PATTERN = os.path.join(DEFINITIONS_DIR, "*.def.h")

# 5. Path to the C Preprocessor (cpp)
#    Try to find it automatically, but you might need to specify it explicitly.
CPP_EXECUTABLE = shutil.which("gcc") # Searches PATH
CLANGFORMAT_EXECUTABLE = shutil.which("clang-format") # Searches PATH
SED_EXECUTABLE = shutil.which("sed") # Searches PATH

# If not found, uncomment and set manually (example paths):
# CPP_EXECUTABLE = "/usr/bin/cpp"
# CPP_EXECUTABLE = "C:/mingw64/bin/cpp.exe"

# 6. Optional flags for cpp (e.g., include paths needed by the headers)
#    Use env.subst to resolve PlatformIO variables like $PROJECT_SRC_DIR
CPP_FLAGS = [
    "-P", # Inhibit generation of #line directives (often useful)
    "-E",
    f"-I{env.subst('$PROJECT_SRC_DIR')}",
    # Add any necessary -I include paths for the template headers here
    # Example: f"-I{env.subst('$PROJECT_LIBDEPS_DIR')}/your_env/lvgl/src", # If templates include LVGL headers directly
    # Add defines if necessary: "-D SOME_DEFINE=1"
]

# --- End Configuration ---

print("--- Custom Preprocessor Script (using cpp) ---")

# --- Essential Path Checks ---
if not CPP_EXECUTABLE:
    print(f"Error: C Preprocessor 'cpp' not found in PATH.")
    print(f"Please install it or set the CPP_EXECUTABLE path manually in extra_script.py.")
    sys.exit(1)
print(f"Using C Preprocessor: {CPP_EXECUTABLE}")

missing_files = False
for header_path in TEMPLATE_HEADERS:
    if not os.path.isfile(header_path):
        print(f"Error: Template header not found: {header_path}")
        missing_files = True

if missing_files:
    print("Error: One or more essential template header files are missing. Exiting.")
    sys.exit(1)

if not os.path.isdir(DEFINITIONS_DIR):
     print(f"Warning: Definitions directory not found, no files to process: {DEFINITIONS_DIR}")

# --- Ensure Output Directory Exists ---
os.makedirs(GENERATED_VIEWS_DIR, exist_ok=True)
print(f"Ensured generated views directory exists: {GENERATED_VIEWS_DIR}")

# --- Find Definition Files ---
definition_file_nodes = env.Glob(DEFINITIONS_PATTERN)

if not definition_file_nodes:
    print(f"No definition files found matching pattern: {DEFINITIONS_PATTERN}")
else:
    print(f"Found {len(definition_file_nodes)} definition file(s) to process.")

def execute(cmd, target_path, name):
    result = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace')

    if result.returncode != 0:
        print(f"    WARN: {name} failed for {os.path.basename(target_path)}!")
        print(f"    --- cpp stdout ---:\n{result.stdout}")
        print(f"    --- cpp stderr ---:\n{result.stderr}")
        # Keep temp file for debugging if cpp fails?
        # print(f"    Temporary input file kept at: {temp_input_file}")
        # temp_input_file = None # Prevent deletion in finally block
    else:
        # Optionally print stderr even on success if it contains warnings
        if result.stderr:
            print(f"    --- {name} stderr (warnings?) ---:\n{result.stderr}")

# --- Python Function Action for Preprocessing ---
def run_cpp_on_combined(target, source, env):
    """
    SCons Action function:
    1. Combines source[0] (template_header) and source[1] (def_file) into a temp file.
    2. Runs cpp on the temp file, outputting to target[0].
    """
    template_h_path = str(source[0])
    def_file_path = str(source[1])
    target_path = str(target)
    cpp_cmd = [CPP_EXECUTABLE] + CPP_FLAGS

    print(f"  Preprocessing combination: {os.path.basename(template_h_path)} + {os.path.basename(def_file_path)} -> {os.path.basename(target_path)}")

    temp_input_file = None
    try:
        result = None
        # Create a temporary file to hold the combined input
        with tempfile.NamedTemporaryFile(mode='w', delete=False, suffix=".h", encoding='utf-8') as tmp_f:
            temp_input_file = tmp_f.name
            # Write template header content
            with open(template_h_path, 'r', encoding='utf-8') as header_f:
                #shutil.copyfileobj(header_f, tmp_f)
                print("APPENDING:", header_f.name)
                tmp_f.write(header_f.read())
            tmp_f.write("\n") # Ensure separation

            # Write definition file content
            with open(def_file_path, 'r', encoding='utf-8') as def_f:
                # shutil.copyfileobj(def_f, tmp_f)
                #print("DEF_H:", def_f.read())
                print("---- APPENDING:", def_f.name)
                tmp_f.write(def_f.read())
            tmp_f.flush()

            #print("TEMP FILE:\n", open(temp_input_file, 'r').read())

            # Construct the full cpp command
            cpp_cmd.extend([temp_input_file, "-o", target_path])

            # Run the C preprocessor
            print(f"    Running: {' '.join(cpp_cmd)}") # Debug command
            result = subprocess.run(cpp_cmd, capture_output=True, text=True, encoding='utf-8', errors='replace')

        if result.returncode != 0:
            print(f"    ERROR: cpp failed for {os.path.basename(target_path)}!")
            print(f"    --- cpp stdout ---:\n{result.stdout}")
            print(f"    --- cpp stderr ---:\n{result.stderr}")
            # Keep temp file for debugging if cpp fails?
            # print(f"    Temporary input file kept at: {temp_input_file}")
            # temp_input_file = None # Prevent deletion in finally block
            return 1 # Indicate failure to SCons
        else:
            # Optionally print stderr even on success if it contains warnings
            if result.stderr:
                 print(f"    --- cpp stderr (warnings?) ---:\n{result.stderr}")

            sed_script = r's/\(#include "[^"]*"\)/\1\n/g'
            sed_cmd = None
            if py_platform.system() == "Darwin":
                sed_cmd = [SED_EXECUTABLE, '-i', '', f"{sed_script}", target_path]
            else:
                sed_cmd = [SED_EXECUTABLE, '-i', f"{sed_script}", target_path]
            print(f"    Running: {' '.join(sed_cmd)}") # Debug command
            execute(sed_cmd, target_path, 'sed post-process')

            clangf_cmd = [CLANGFORMAT_EXECUTABLE, '-i', target_path]
            print(f"    Running: {' '.join(clangf_cmd)}") # Debug command
            execute(clangf_cmd, target_path, 'clang-format')

            with open(target_path, 'r+') as file:
                existing_content = file.read()
                file.seek(0, 0)
                file.write("/**\n")
                file.write("* Auto-Generated and OVERWRITTEN --- DO NOT EDIT THIS FILE\n")
                file.write("*\n")
                file.write("* Edit the corresponding '*.def.h' in 'src/ui/components/'.\n")
                file.write("**/\n")
                file.write("\n")
                file.write(existing_content)

    except Exception as e:
        print(f"  ERROR: Exception during preprocessing for {os.path.basename(target_path)}: {e}")
        traceback.print_exc(file=sys.stdout)
        return 1 # Indicate failure
    finally:
        # Clean up the temporary file if it was created and not kept for debugging
        if temp_input_file and os.path.exists(temp_input_file):
            try:
                os.remove(temp_input_file)
            except OSError as e:
                print(f"    Warning: Could not remove temporary file {temp_input_file}: {e}")

    return 0 # Indicate success to SCons

# --- Process Each Combination ---
all_generated_nodes_c = []
all_generated_paths = []

for def_node in definition_file_nodes:
    def_file_path = def_node.path
    def_basename = os.path.basename(def_file_path)
    # Get the core name (e.g., "bar_value_view" from "bar_value_view.def.h")
    core_name = def_basename.replace(".def.h", "")
    if core_name == def_basename:
        print(f"Warning: Could not extract core name from '{def_basename}'. Skipping.")
        continue

    print(f"\nProcessing Definition: {def_basename} (Core Name: {core_name})")

    for template_h_path in TEMPLATE_HEADERS:
        template_basename = os.path.basename(template_h_path)
        template_name_part = os.path.splitext(template_basename)[0]

        # Determine output extension based on template name
        if template_name_part.endswith("_def"):
            output_ext = ".h"
        elif template_name_part.endswith("_impl"):
            output_ext = ".c"
        else:
            output_ext = ".pre-out" # Fallback extension

        output_name = f"{core_name}{output_ext}" # Changed naming convention slightly
        output_path = os.path.join(GENERATED_VIEWS_DIR, output_name)
        all_generated_paths.append(output_path)
        
        run_cpp_on_combined(output_path, [template_h_path, def_node], env)
        # Define the SCons Command using the Python function action
        #cmd_gen = env.Command(
        #    target=output_path,
        #    source=[template_h_path, def_node], # Depends on the template AND the specific def file
        #    action=env.Action(run_cpp_on_combined, cmdstr=f"Preprocessing {os.path.basename(output_path)}")
        #)
        # Optional: Add dependency on the cpp executable itself?
        # env.Depends(cmd_gen, CPP_EXECUTABLE)

        # If it's a C file, add its Node to the list for compilation
        #if output_ext == ".c":
        #    all_generated_nodes_c.append(cmd_gen[0]) # cmd_gen returns a list of target Nodes

# --- Update Build Environment ---

# 1. Add all generated C files to the list of files to be compiled
#if all_generated_nodes_c:
#    env.Append(PIOBUILDFILES=all_generated_nodes_c)
#    print(f"\nAdded {len(all_generated_nodes_c)} generated C file(s) to build sources.")
#else:
#    print("\nNo generated C files to add to build sources.")

# 2. Add the generated directory to the include paths
#    Needed so the generated .c files (or other code) can find the generated .h files
env.Append(CPPPATH=[GENERATED_VIEWS_DIR])
print(f"Added generated directory to CPPPATH (include paths): {GENERATED_VIEWS_DIR}")

# 3. Add generated files to the clean target
if all_generated_paths:
    env.Clean(env.subst("$PROGNAME"), all_generated_paths)
    print(f"Added {len(all_generated_paths)} potential generated files to clean target.")
    # env.Clean(env.subst("$PROGNAME"), GENERATED_VIEWS_DIR) # Optionally clean dir

print("--- Custom Preprocessor Script Finished ---")