# Collect all example source files
file(GLOB EXAMPLE_SOURCES "*.cpp")

foreach(example_src ${EXAMPLE_SOURCES})
    get_filename_component(example_name ${example_src} NAME_WE)
    add_executable(${example_name} ${example_src})
    target_link_libraries(${example_name} PRIVATE ${PROJECT_NAME})
endforeach()
