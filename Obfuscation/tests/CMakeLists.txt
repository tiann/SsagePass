file(GLOB test_files "*.cpp")

set(PASSES "function(bcf)" "function(mba)" "function(lower-switch)" "function(split)" "function(vmf)" "function(icall)" "ipobf" "indibr" "funwra" "enfla" "strenc" "strcry")
set(TMPDIR "${PROJECT_BINARY_DIR}/Testing/Temporary")

function(add_pass_test TESTFILENAME PASS IDX)
  add_test(
    NAME "${TESTFILENAME}_opt_${IDX}"
    COMMAND ${LT_LLVM_INSTALL_DIR}/bin/opt -opaque-pointers=0 -load-pass-plugin ${PROJECT_BINARY_DIR}/libSsageObfuscator.so "-passes=${PASS}" "${TMPDIR}/${TESTFILENAME}.ll" -o "${TMPDIR}/${TESTFILENAME}.bc"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )

  add_test(
    NAME "${TESTFILENAME}_compile_${IDX}"
    COMMAND ${LT_LLVM_INSTALL_DIR}/bin/clang++ "${TMPDIR}/${TESTFILENAME}.bc" -o "${TMPDIR}/${TESTFILENAME}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )

  add_test(
    NAME "${TESTFILENAME}_run_${IDX}"
    COMMAND "${TMPDIR}/${TESTFILENAME}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )
endfunction()

foreach(TESTFILE IN LISTS test_files)
  get_filename_component(TESTFILENAME "${TESTFILE}" NAME_WLE)

  add_test(
    NAME "${TESTFILENAME}_emit_llvm"
    COMMAND ${LT_LLVM_INSTALL_DIR}/bin/clang++ -O3 -S -emit-llvm -Xclang -no-opaque-pointers "${TESTFILENAME}.cpp" -o "${TMPDIR}/${TESTFILENAME}.ll"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )

  set(COUNTER 0)
  foreach(PASS IN LISTS PASSES)
    add_pass_test(${TESTFILENAME} "${PASS}" "${COUNTER}")
    MATH(EXPR COUNTER "${COUNTER}+1")
  endforeach()
endforeach()
