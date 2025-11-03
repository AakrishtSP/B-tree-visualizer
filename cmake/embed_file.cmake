# Script to embed a binary file as a C array

file(READ ${INPUT_FILE} file_contents HEX)

# Convert hex to array of bytes
string(REGEX MATCHALL "([0-9a-f][0-9a-f])" bytes ${file_contents})

# Create the header file
set(output_content "// Auto-generated file - do not edit\n")
string(APPEND output_content "#ifndef EMBEDDED_FONT_H\n")
string(APPEND output_content "#define EMBEDDED_FONT_H\n\n")
string(APPEND output_content "static const unsigned char ${VARIABLE_NAME}[] = {\n")

# Write bytes in rows of 16
list(LENGTH bytes byte_count)
math(EXPR last_index "${byte_count} - 1")

set(index 0)
foreach(byte ${bytes})
    if(index GREATER 0)
        string(APPEND output_content ",")
        math(EXPR mod_result "${index} % 16")
        if(mod_result EQUAL 0)
            string(APPEND output_content "\n")
        else()
            string(APPEND output_content " ")
        endif()
    else()
        string(APPEND output_content "    ")
    endif()
    
    string(APPEND output_content "0x${byte}")
    math(EXPR index "${index} + 1")
endforeach()

string(APPEND output_content "\n};\n\n")
string(APPEND output_content "static const unsigned int ${VARIABLE_NAME}_size = ${byte_count};\n\n")
string(APPEND output_content "#endif // EMBEDDED_FONT_H\n")

file(WRITE ${OUTPUT_FILE} "${output_content}")
