cmake_minimum_required(VERSION 3.1)
PROJECT(cpp20_orderbook)

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

INCLUDE_DIRECTORIES(include)

SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

SET(SOURCES 
  src/lib.cpp)

ADD_EXECUTABLE(test_async tests/test_async.cpp ${SOURCES})
TARGET_LINK_LIBRARIES(test_async)

ADD_EXECUTABLE(test_generator_istream tests/test_generator_istream.cpp ${SOURCES})
TARGET_LINK_LIBRARIES(test_generator_istream)

ADD_EXECUTABLE(test_lib tests/test_lib.cpp ${SOURCES})
TARGET_LINK_LIBRARIES(test_lib)

ADD_EXECUTABLE(test_parser tests/test_parser.cpp ${SOURCES})
TARGET_LINK_LIBRARIES(test_parser)

ADD_EXECUTABLE(run_app src/main.cpp ${SOURCES})
TARGET_LINK_LIBRARIES(run_app)

ENABLE_TESTING()
ADD_TEST(AsyncTests bin/test_async)
ADD_TEST(GeneratorIStream bin/test_generator_istream)
ADD_TEST(LibTests bin/test_lib)
ADD_TEST(ParserTests bin/test_parser)