# step 1: '-pg' option is needed in both compiling and linking
gcc gprof_test.c -pg

# step 2: run the exec file, this will generate gmon.out
./a.out

# step 3: run gprof, this will print the profiling info to stdout
gprof
