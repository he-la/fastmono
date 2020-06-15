_NOTE_: Consider all statements made here *Retracted* - the below was written under an abundance of confidence. I was made aware that the algorithm is flawed even on a conceptual level and will not work or achieve the desired asymptotic complexity. I originally decided the algorithm to tackle fast triangulation of complex (self-intersecting) polygons. I still believe that monotone partitioning is the right approach to do this in principle, however when trying to fix the issues in this algorithm, I realised that I was essentially reinventing Seidel's algorithm. As such, Seidel's algorithm is probably a much better approach also for complex polygons, but I have lost interest in pursuing this problem further.

----

_NOTE_: Due to shortcuts in the internal representation of diagonals, certain
polygons can cause the library to crash with a segfault. This should be
considered a critical bug and use of this library is strongly discouraged until
I find the time to fix these issues.

# FastMono
Monotone polygon triangulation on steroids.

No triangulation algorithm is perfect. This header-only library aims to provide
a compromise between simplicity and performance, yielding an n log n algorithm
at a remarkably low constant factor.

The algorithm used improves on the popular monotone triangulation algorithm by
Garey et al. by reducing the number of sorts to only the required cases.
Revisiting the approach from scratch, this algorithm employs a selective lookup
strategy using red-black trees and simple binary search trees. Thereby, the
number of expensive n log n operations is reduced to the number of vertices that
break the monotone constraint.

While being asymptotically inferior, using monotone partitioning has decisive
advantages over approaches such as Seidel's trapezoidation technique: With
comfortably less than a thousand lines of code, the algorithm is incredibly
simple. This makes it easy to maintain and extend. Moreover, since monotone
polygons have well-defined properties, the partitioning algorithm can be reused
in other algorithms.

For example, it should be easily possible to extend the library to edge cases
such as polygons with holes or complex polygons with self-intersections.

## Building
This is a header-only library; you only need to `#include "polygon.hpp"`.

### Options
If you wish to build the library without runtime input checks and stdexcept
error handling, you may define the `FMT_NOEXCEPT` macro. This is not
recommended. Note that defining `DEBUG` automatically undefines `FMT_NOEXCEPT`
and enables additional runtime checks.

### Documentation
You can generate the documentation in your desired format using
[Doxygen](http://www.doxygen.org). By default `doxygen` generates LaTeX and
html.

### Building the benchmarks
This library uses [CMake](https://cmake.org) to generate appropriate build
files. To build the benchmarks, `cd` to the benchmarks subdirectory and run the
following commands:

``` shell
mkdir build && cd build
cmake ..
cmake --build .
```

By default, if your compiler supports LLVM polly, appropriate compiler flags
will be generated. If you wish to disable this behaviour, add `-DFMT_NO_POLLY`
to the cmake invocation. Note that if you have previously built the benchmarks
with support for polly, you will need to first clean the build directory.

## TODO
This library is in alpha and very much a work in progress (once I find the
time). It currently lacks automated tests, which means that the library may or
may not be bug-free, and may possibly not be implemented in perfect accordance
with the specification. Moreover, the interface through which the librar is used
may change. The current todo list includes:
- [x] Benchmarks
- [ ] Tests
- [ ] Algorithm description in a human language
- [ ] Complex Polygons
- [ ] Refactoring the partition function into smaller methods for better testing
- [ ] More optimisations
