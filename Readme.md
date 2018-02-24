# FastMono
Monotone polygon triangulation on steroids.

No triangulation algorithm is perfect. This header-only library aims
to provide a compromise between simplicity and performance, yielding
an n log n algorithm at a remarkably low constant factor.

The algorithm used improves on the popular monotone triangulation
algorithm by Garey et al. by reducing the number of sorts to only the
required cases. Revisiting the approach from scratch, this algorithm
employs a selective lookup strategy using red-black trees and simple
binary search trees. Thereby, the number of expensive n log n
operations is reduced to the number of vertices that break the
monotone constraint. In other words, as a given polygon becomes more
monotone on the axis of triangulation (currently the x axis), the
triangulation performance approaches linear time.

The algorithm currently does not handle polygons with holes or
self-intersections (complex polygons), although support for both can
theoretically be implemented.

## Building
This is a header-only library; you only need to #include
"polygon.hpp".

### Options
If you wish to build the library without runtime input checks and
stdexcept error handling, you may define the `FMT_NOEXCEPT` macro.
This is not recommended. Note that defining `DEBUG` automatically
undefines `FMT_NOEXCEPT` and enables additional runtime checks.

### Documentation
You can generate the documentation in your desired format using
[Doxygen](http://www.doxygen.org). By default `doxygen` generates
LaTeX and html.

### Building the benchmarks
This library uses [CMake](https://cmake.org) to generate appropriate
build files. To build the benchmarks, `cd` to the benchmarks
subdirectory and run the following commands:

``` shell
mkdir build && cd build
cmake ..
cmake --build .
```

## TODO
This library is in alpha and very much a work in progress (once I find
the time). It currently lacks automated tests, which means that the
library may or may not be bug-free, and may possibly not be
implemented in perfect accordance with the specification. Moreover,
the interface through which the librar is used may change. The current
todo list includes:
- [x] Benchmarks
- [ ] Tests
- [ ] Algorithm description in a human language
- [ ] Complex Polygons
- [ ] Refactoring the partition function into smaller methods for better
testing
- [ ] More optimisations
