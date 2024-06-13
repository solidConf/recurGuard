# RecurGuard
RecurGuard is a oracle generation approach for non-terminating recursion. By RecurGuard, non-terminating recursions in real-wolrd projects are able to be detected. Please refer to our website for more information on our work and can also find the results of our experiment.
Webset of https://sites.google.com/view/infiniterecursion/home


## How to use
Our method is realized in ./llvm-mode

### Build AFL and RecurGuard wrapper:
```
$cd ProjectFile
$make
$cd llvm-mode
$make
```

### Testing on program

Build the project by our wrapper "RecurGuard" (c project), and "RecurGuard++" (c++ project).

For example for a case program.c
```
$./RecurGuard program.c
```
Then run afl-fuzz to fuzzing on the project.
```
$./afl-fuzz -i [input] -o [out] program
```
Finally, you can find the non-termination report for the project in "out/non_term_recursion" file.
