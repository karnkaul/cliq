add_executable(cliqr-lab)

target_link_libraries(cliqr-lab PRIVATE cliqr)

target_sources(cliqr-lab PRIVATE main.cpp)

target_compile_features(cliqr-lab PRIVATE cxx_std_23)
