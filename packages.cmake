include(FetchContent)

cmake_policy(PUSH)
cmake_policy(SET CMP0150 NEW)

FetchContent_Declare(commline
    GIT_REPOSITORY ../commline.git
    GIT_TAG        aea2cd146b654e28db102ba7c6b0489bfbc1ef57 # 0.4.0
)

FetchContent_Declare(dotenv
    GIT_REPOSITORY ../dotenv-cpp.git
    GIT_TAG        49c5e254e03ec6914ad1de34be3bd66ed729f426 # branch: main
)

FetchContent_Declare(ext
    GIT_REPOSITORY ../libext.git
    GIT_TAG        76265c1325028676ae3219505bb362a0b28ad1ea # 0.3.0
)

set(FMT_INSTALL ON)
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        a33701196adfad74917046096bf5a2aa0ab0bb50 # 9.1.0
)

FetchContent_Declare(GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        b10fad38c4026a29ea6561ab15fc4818170d1c10 # branch: main
)

set(JSON_Install ON)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        bc889afb4c5bf1c0d8ee29ef35eaaf4c8bef8a5d # v3.11.2
)

FetchContent_Declare(netcore
    GIT_REPOSITORY ../netcore.git
    GIT_TAG        287a0ea7c20d8549fdf01a6eb55931fdcc5d58ff # 0.5.0
)

FetchContent_Declare(timber
    GIT_REPOSITORY ../timber.git
    GIT_TAG        9e6fd332fc3dc80a14ad8d5476a268ea867714f0 # 0.4.0
)

FetchContent_Declare(uuidcpp
    GIT_REPOSITORY ../uuidcpp.git
    GIT_TAG        55349429b9b7f9b671f8e64a01963017880f5232 # 0.3.0
)

FetchContent_MakeAvailable(
    commline
    ext
    fmt
    nlohmann_json
    netcore
    timber
    uuidcpp
)

if(BUILD_EXAMPLES OR BUILD_UTILITIES)
    FetchContent_MakeAvailable(dotenv)
endif()

if(PROJECT_TESTING)
    FetchContent_MakeAvailable(dotenv GTest)
endif()

cmake_policy(POP)
