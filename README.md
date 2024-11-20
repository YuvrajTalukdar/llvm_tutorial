Some basic codes for llvm

I have tested the codes on llvm 18.1.8.

1. ir_experiments- contains code related to ir ast assembly dumps.
2. experiment1- clang plugin experiments.
3. experiment2- clang tooling experiments.
4. experiment3- clang tooling code with cuda support
5. experiment4- clang tooling code for parsing and printing cuda program.
6. experiment5- clang tooling code for encaptulating a statement present inside a cuda kernel in a loop.

Instructions For experiment1 to experiment5
1. Use Arch Linux, and be based.
2. Install llvm llvm-libs clang: `sudo pacman -S llvm llvm-libs clang`
3. Clone this repo: `git clone https://github.com/YuvrajTalukdar/llvm_tutorial.git`
4. cd to any of the experiment directory and read the Notes. Say experiment5
5. once inside the experiment directory do `mkdit build`
6. `cd build`
7. `cmake ..`
8. `make`
9. Execute the compiled binary while passing the program you want to parse.
