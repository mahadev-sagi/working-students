# Each command starts a fresh process, matching deployment key rotation.
set(key_a "test-only-key-a-0123456789abcdef0123456789abcdef")
set(key_b "test-only-key-b-0123456789abcdef0123456789abcdef")

foreach(invalid "" "too-short" "replace-this-with-secure-env-secret" "                                ")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "JWT_SECRET=${invalid}"
        "${TEST_EXECUTABLE}" issue RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT result STREQUAL "1" OR NOT output STREQUAL "" OR
       NOT error MATCHES "^JWT_SECRET must be set to a new random secret")
        message(FATAL_ERROR "Invalid JWT configuration did not fail closed")
    endif()
endforeach()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env --unset=JWT_SECRET
    "${TEST_EXECUTABLE}" issue RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result STREQUAL "1" OR NOT output STREQUAL "" OR
   NOT error MATCHES "^JWT_SECRET must be set to a new random secret")
    message(FATAL_ERROR "Missing JWT configuration did not fail closed")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E env "JWT_SECRET=${key_a}"
    "${TEST_EXECUTABLE}" roundtrip RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result STREQUAL "0" OR NOT error STREQUAL "" OR
   NOT output STREQUAL "Authentication regression checks passed\n")
    message(FATAL_ERROR "Authentication regression or unexpected login logging")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -E env "JWT_SECRET=${key_a}"
    "${TEST_EXECUTABLE}" issue RESULT_VARIABLE result OUTPUT_VARIABLE token ERROR_VARIABLE error)
if(NOT result STREQUAL "0" OR NOT error STREQUAL "")
    message(FATAL_ERROR "Token issuance failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "JWT_SECRET=${key_a}"
    "${TEST_EXECUTABLE}" verify "${token}" RESULT_VARIABLE result)
if(NOT result STREQUAL "0")
    message(FATAL_ERROR "Original signing key rejected its own token")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E env "JWT_SECRET=${key_b}"
    "${TEST_EXECUTABLE}" verify "${token}" RESULT_VARIABLE result)
if(NOT result STREQUAL "1")
    message(FATAL_ERROR "Rotated signing key accepted an old token")
endif()
message(STATUS "JWT configuration, login, forgery rejection, and rotation checks passed")
