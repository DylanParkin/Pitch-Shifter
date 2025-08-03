MECHENG 313 Assignment 2:

Johnson Du
jdu853

Dylan Parkin
dpar783

For more information about this project view "MECHENG 313 A2 REPORT.pdf".

Compilation and Execution Instructions for Windows and Mac OS (replace x with task number):
---

Step 1: Install PortAudio

On Windows:
  Install PortAudio manually or using MSYS2/vcpkg, and use a compiler like MinGW.

On Mac:
  Install PortAudio using Homebrew: brew install portaudio

---

Step 2: Compile the Program

Navigate to the folder containing the source file.

On Windows (with MSYS2 and PortAudio installed):
  g++ task2.x.cpp smbPitchShift.cpp -lportaudio -o task2.x.exe

On macOS with Homebrew:
  g++ task2.x.cpp smbPitchShift.cpp \
    -I/opt/homebrew/opt/portaudio/include \
    -L/opt/homebrew/opt/portaudio/lib \
    -lportaudio -o task2.x

---

Step 3: Run the Program

On Windows:
  task2.x

On Mac:
  ./task2.x
