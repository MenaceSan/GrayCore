
# Collect all   headers
file(GLOB_RECURSE PUBLIC_CPP
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)

target_sources(graycore PRIVATE ${PUBLIC_CPP})
