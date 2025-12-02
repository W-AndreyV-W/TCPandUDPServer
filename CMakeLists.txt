cmake_minimum_required (VERSION 3.10)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("TCPandUDPServer")

add_executable (TCPandUDPServer "TCPandUDPServer.cpp" 
								"MultiplexingSocket.h" 
								"MultiplexingSocket.cpp"  
								"MultiplexingSocketTCP.h" 
								"MultiplexingSocketTCP.cpp" 
								"MultiplexingSocketUDP.h"
								"MultiplexingSocketUDP.cpp"
								"MessageProcessing.h" 
								"MessageProcessing.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TCPandUDPServer PROPERTY CXX_STANDARD 20)
endif()