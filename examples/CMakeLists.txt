project(cliq-examples)

function(create_executable NAME MAIN)
  add_executable(${NAME})
  target_link_libraries(${NAME} PRIVATE cliq::cliq)
  target_sources(${NAME} PRIVATE "${MAIN}")
endfunction()

create_executable(cliq-multiplier command_app.cpp)
create_executable(cliq-calculator command_list_app.cpp)
create_executable(cliq-basic basic.cpp)
create_executable(cliq-commands commands.cpp)
