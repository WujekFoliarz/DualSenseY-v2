include_directories(..)

add_executable(tpl_io_test io_test.cpp)
target_link_libraries(tpl_io_test tiny-process-library)
add_test(tpl_io_test tpl_io_test)

add_executable(tpl_open_close_test open_close_test.cpp)
target_link_libraries(tpl_open_close_test tiny-process-library)
add_test(tpl_open_close_test tpl_open_close_test)

add_executable(tpl_multithread_test multithread_test.cpp)
target_link_libraries(tpl_multithread_test tiny-process-library)
add_test(tpl_multithread_test tpl_multithread_test)

add_executable(tpl_path_test path_test.cpp)
target_link_libraries(tpl_path_test tiny-process-library)
add_test(tpl_path_test tpl_path_test)
