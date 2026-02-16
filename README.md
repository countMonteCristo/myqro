![QR-code example](qr.png)

# myqro
Yet another QR code generation library in C++

This library was written moslty in educational purposes. Use it in your own risk.

## Getting started
### Dependencies
* gcc or clang or other c++ compiler
* cmake

### Testing dependencies
* cppunit

### Installation
1. Install dependencies:
```sh
$ sudo apt-get install gcc cmake libcppunit
```

2. Clone the repo:
```sh
git clone https://github.com/countMonteCristo/myqro.git
```

3. Build project
```sh
mkdir build && cd build
cmake ..
make -i myqro_static myqro_shared
```

4. Run unit tests (from **build** folder)
```sh
make myqro-unit-test
../bin/myqro-unit-test
```

5. Build and run **encoder**
```
make encoder
../bin/encoder -e bytes -c M -m 1 "Hello, world!"
```

## Contacts
Artem Shapovalov: artem_shapovalov@frtk.ru

Project Link: https://github.com/countMonteCristo/myqro