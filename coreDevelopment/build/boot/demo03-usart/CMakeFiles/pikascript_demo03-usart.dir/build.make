# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/ubuntu/pikascript/coreDevelopment

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/ubuntu/pikascript/coreDevelopment/build

# Include any dependencies generated for this target.
include boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/depend.make

# Include the progress variables for this target.
include boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/progress.make

# Include the compile flags for this target's objects.
include boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/flags.make

boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/main.c.o: boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/flags.make
boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/main.c.o: ../boot/demo03-usart/main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/ubuntu/pikascript/coreDevelopment/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/main.c.o"
	cd /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/pikascript_demo03-usart.dir/main.c.o   -c /home/ubuntu/pikascript/coreDevelopment/boot/demo03-usart/main.c

boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/pikascript_demo03-usart.dir/main.c.i"
	cd /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/ubuntu/pikascript/coreDevelopment/boot/demo03-usart/main.c > CMakeFiles/pikascript_demo03-usart.dir/main.c.i

boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/pikascript_demo03-usart.dir/main.c.s"
	cd /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/ubuntu/pikascript/coreDevelopment/boot/demo03-usart/main.c -o CMakeFiles/pikascript_demo03-usart.dir/main.c.s

# Object files for target pikascript_demo03-usart
pikascript_demo03__usart_OBJECTS = \
"CMakeFiles/pikascript_demo03-usart.dir/main.c.o"

# External object files for target pikascript_demo03-usart
pikascript_demo03__usart_EXTERNAL_OBJECTS =

boot/demo03-usart/pikascript_demo03-usart: boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/main.c.o
boot/demo03-usart/pikascript_demo03-usart: boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/build.make
boot/demo03-usart/pikascript_demo03-usart: package/pikascript/libpikascript-core.a
boot/demo03-usart/pikascript_demo03-usart: boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/ubuntu/pikascript/coreDevelopment/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable pikascript_demo03-usart"
	cd /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/pikascript_demo03-usart.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/build: boot/demo03-usart/pikascript_demo03-usart

.PHONY : boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/build

boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/clean:
	cd /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart && $(CMAKE_COMMAND) -P CMakeFiles/pikascript_demo03-usart.dir/cmake_clean.cmake
.PHONY : boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/clean

boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/depend:
	cd /home/ubuntu/pikascript/coreDevelopment/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/ubuntu/pikascript/coreDevelopment /home/ubuntu/pikascript/coreDevelopment/boot/demo03-usart /home/ubuntu/pikascript/coreDevelopment/build /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart /home/ubuntu/pikascript/coreDevelopment/build/boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : boot/demo03-usart/CMakeFiles/pikascript_demo03-usart.dir/depend

