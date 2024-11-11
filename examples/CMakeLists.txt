project(cliq-examples)

function(create_executable NAME MAIN)
  add_executable(${NAME})
  target_link_libraries(${NAME} PRIVATE cliq::cliq)
  target_sources(${NAME} PRIVATE "${MAIN}")
  target_compile_features(${NAME} PRIVATE cxx_std_23)
endfunction()

create_executable(cliq-multiplier command_app.cpp)
create_executable(cliq-calculator command_list_app.cpp)
