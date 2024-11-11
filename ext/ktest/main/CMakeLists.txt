add_library(${PROJECT_NAME}-main)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-main ALIAS ${PROJECT_NAME}-main)

target_link_libraries(${PROJECT_NAME}-main PRIVATE
  ${PROJECT_NAME}::${PROJECT_NAME}-compile-options
)

target_link_libraries(${PROJECT_NAME}-main PUBLIC
  ${PROJECT_NAME}::${PROJECT_NAME}
)

target_sources(${PROJECT_NAME}-main PRIVATE
  ktest_main.cpp
)
