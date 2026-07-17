function(aegis_add_test test_name source_file)
  qt_add_executable(${test_name} "unit/${source_file}")
  target_link_libraries(${test_name} PRIVATE aegis_core Qt6::Test Qt6::Network)
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
  )
endfunction()

aegis_add_test(tst_pathguard tst_pathguard.cpp)
aegis_add_test(tst_atomicfile tst_atomicfile.cpp)
aegis_add_test(tst_aegiserror tst_aegiserror.cpp)
aegis_add_test(tst_httpclient tst_httpclient.cpp)
