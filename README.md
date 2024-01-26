# Base Template for C++ based projects
## How to use

TODO: section on how to initialize with this 

## Name and Version

In order to specify the project name, executable name and name of the unit tests executable, edit the CMakeLists.txt.
In there, it says:

```cmake
project(
    "MY_PROJECT" # replace this
    VERSION 0.1.0
    LANGUAGES CXX
)
```

Edit the `MY_PROJECT` to a name that suits your project, and adjust the `VERSION` if necessary.

Further, go into `vcpkg.json` and edit the two lines shown here, similarly to how you did in the CMakeLists.txt:

```json
{
  "name": "my-project",
  "version-string": "0.1.0",
  ...
```

Keep in mind the name constraints and requirements for vcpkg.

## Adding source files

To add source files, add them to the following section in the CMakeLists.txt:

```cmake
# add all headers (.h, .hpp) to this
set(PRJ_HEADERS )
# add all source files (.cpp) to this, except the one with main()
set(PRJ_SOURCES )
```
 
For example, to add `mysource.cpp`, `somefile.cpp` and `myheader.h`, proceed as follows:

```cmake
# add all headers (.h, .hpp) to this
set(PRJ_HEADERS mysource.cpp somefile.cpp)
# add all source files (.cpp) to this, except the one with main()
set(PRJ_SOURCES myheader.h)
```

## Building

To build, run cmake (`bin` will be the output directory, `.` the source directory):

```sh
cmake -S . -B bin
```

This will configure it with all default settings.
Then build with:

```sh
cmake --build bin --parallel
```

## Adding dependencies

First, find your dependency on vcpkg (for example using [this website](https://vcpkg.io/en/packages.html)). For example, you may be looking for lionkor's `commandline` library, and you will find `lionkor-commandline`.

Second, note down the name, in our example `lionkor-commandline`, and add it to the vcpkg.json, like so:

Before:
```json
{
  ...
  "dependencies": [
      "fmt",
      "doctest"
  ]
}
```

After:
```json
{
  ...
  "dependencies": [
      "fmt",
      "doctest",
      "lionkor-commandline"
  ]
}
```


Third, go to CMakeLists.txt and find the following:

```cmake
# add dependency find_package calls and similar here
find_package(fmt CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
...
```
and add your `find_package` call or equivalent below it.

Last, find this section (above the previous):
```cmake
# add all libraries used by the project (WARNING: also set them in vcpkg.json!)
set(PRJ_LIBRARIES 
    fmt::fmt
    ...
)
```
and add your library there. The exact expression used here differs from library to library, but should be easy to find in their documentation. When in doubt, it's simply the name of the library.




