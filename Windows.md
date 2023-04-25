## GNU (msys2)

### Compiler

#### Use Existing one
```bash
pacman -S mingw-w64-clang-x86_64-llvm mingw-w64-clang-x86_64-clang mingw-w64-clang-x86_64-ninja mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-eigen3
```

#### Build yourself
```bash
cmake -S llvm -B build-gnu -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DLLVM_INSTALL_UTILS=ON -DLLVM_ENABLE_PROJECTS="llvm;clang;lld" -DLLVM_STATIC_LINK_CXX_STDLIB=ON -DLLVM_ENABLE_ZLIB=OFF -DLLVM_ENABLE_LIBXML2=OFF
cmake --build build-gnu -j $(nproc)

#some passes dependency
pacman -S mingw-w64-clang-x86_64-eigen3
```

### Build passes

```bash
cmake -S . -B build-gnu -G Ninja -DLT_LLVM_INSTALL_DIR="/clang64" -DCMAKE_BUILD_TYPE=release
```
