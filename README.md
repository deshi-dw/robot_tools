Various Tools for controlling and configuring robots.

# robot_tools

This is an attempt at making an application that interacts with the robot in a way more complex that the original FRC Drive Station.

Of course, the FRC Drive Station will still be required to run the robot but this application will try to add custom networking, inputs, and camera viewer. 

## Requirements

- Windows only because of the HID code. This could be solved in the future.
- Windows Driver Kit (https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/)
- Clang compiler for Windows (https://github.com/llvm/llvm-project)
- A mingw toolchain (https://github.com/mstorsjo/llvm-mingw)

## Status

Currently working on inputs. No other functionality works.
