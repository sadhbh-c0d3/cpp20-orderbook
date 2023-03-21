# C++20 Order Book

## About

Fully versatile Order Book in C++20:
- implemented using latest greatest features of C++20
- 2nd order templates, concepts, coroutines

## Building

1. Launch Docker container
```
    docker-compose up -d
```

2. Enter development environment within Docker container
```
    ./enter-app.sh
```

3. Configure
```
    mkdir /home/build
    cd /home/build
    cmake /home/app
```

4. Build
```
    make
```

5. Test
```
    ctest
```

6. Run App
```
    ./bin/run_app
```
