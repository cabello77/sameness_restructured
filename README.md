# Sameness

Restructured C++ project with a clean layout:

```
.
├── CMakeLists.txt
├── include/        # Public headers
├── src/            # Source files
├── tests/          # Unit tests
└── third_party/    # External dependencies
```

## Building

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

## Running

The main executable will be `sameness` in the build folder.
