# add the executable
add_executable (HelloWorld hello_world.cc)
target_link_libraries (HelloWorld HelloClasses)

install (
  TARGETS
    HelloWorld
  EXPORT
    helloworld-targest
  DESTINATION
    bin
)

add_test(NAME HelloWorldExec COMMAND HelloWorld)
