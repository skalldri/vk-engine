
add_library(fmtlog STATIC)

target_include_directories(
    fmtlog
    PUBLIC
        include
)

target_include_directories(
    fmtlog
    PRIVATE
        .
)

target_sources(
    fmtlog
    PRIVATE
        Log.cpp
)

target_link_libraries(
    fmtlog
    PUBLIC
        fmt::fmt
)