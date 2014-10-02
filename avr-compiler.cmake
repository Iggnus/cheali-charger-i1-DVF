
SET(CSTANDARD "-std=gnu99")
SET(CDEBUG "-g -gdwarf-2")
SET(CWARN "-Wall")
SET(CTUNING "-funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums")
SET(COPT "-Os")
SET(CINCS "")
SET(CMCU "-mmcu=atmega32")
SET(CDEFS "-DF_CPU=16000000")

SET(CXXEXTRA "-ffunction-sections -fdata-sections -fno-exceptions")
#-D__IN_ECLIPSE__=1 -Wall -Os -ffunction-sections -fdata-sections -fno-exceptions -g -mmcu=atmega32 -DF_CPU=16000000UL -MMD -MP -MF"AnalogInputs.d" -MT"AnalogInputs.d" -c -o "AnalogInputs.o" -x c++ "../AnalogInputs.cpp"



SET(CFLAGS "${CMCU} ${CDEBUG} ${CDEFS} ${CINCS} ${COPT} ${CWARN} ${CSTANDARD} ${CTUNING} ${CEXTRA} ${CXXEXTRA}")
SET(CXXFLAGS "${CMCU} ${CDEBUG} ${CDEFS} ${CINCS} ${COPT} ${CXXEXTRA} ${CTUNING}")

SET(CMAKE_C_FLAGS  ${CFLAGS})
SET(CMAKE_CXX_FLAGS ${CXXFLAGS})
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--gc-sections -lm")

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

