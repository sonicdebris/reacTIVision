cmake_minimum_required(VERSION 3.10.2)

set(rv_src ../../../../../common/)

set(tinyxml_src ../../../../../ext/portvideo/tinyxml)
add_library(tinyxml STATIC ${tinyxml_src}/tinyxml2.cpp )
target_include_directories(tinyxml PUBLIC ${tinyxml_src})

set(osc_src ../../../../../ext/oscpack/)
add_library(osc STATIC
        ${osc_src}/osc/OscOutboundPacketStream.cpp  ${osc_src}/osc/OscReceivedElements.cpp
        ${osc_src}/osc/OscPrintReceivedElements.cpp ${osc_src}/osc/OscTypes.cpp
        ${osc_src}/ip/IpEndpointName.cpp            ${osc_src}/ip/posix/NetworkingUtils.cpp
        ${osc_src}/ip/posix/UdpSocket.cpp)

target_include_directories(osc PUBLIC ../../../../../ext/oscpack/)

set(tuio_src ../../../../../ext/tuio)
add_library(tuio STATIC
        ${tuio_src}/TuioContainer.cpp   ${tuio_src}/TuioObject.cpp
        ${tuio_src}/UdpSender.cpp       ${tuio_src}/OneEuroFilter.cpp   ${tuio_src}/TuioCursor.cpp
        ${tuio_src}/TuioPoint.cpp       ${tuio_src}/WebSockSender.cpp   ${tuio_src}/TcpSender.cpp
        ${tuio_src}/TuioDispatcher.cpp  ${tuio_src}/TuioServer.cpp      ${tuio_src}/TuioBlob.cpp
        ${tuio_src}/TuioManager.cpp     ${tuio_src}/TuioTime.cpp )
target_include_directories(tuio PUBLIC ${tuio_src})
target_link_libraries(tuio osc log)

set(portvideo_src ../../../../../ext/portvideo/common)
add_library(portvideo STATIC ${portvideo_src}/ConsoleInterface.cpp )
target_include_directories(portvideo PRIVATE ${rv_src})
target_include_directories(portvideo PUBLIC ${portvideo_src})
target_link_libraries(portvideo tinyxml)

set(fid_src ../../../../../ext/libfidtrack)
add_library(fidtrack STATIC
        ${fid_src}/dump_graph.c  ${fid_src}/segment.c                   ${fid_src}/topologysearch.c
        ${fid_src}/fidtrackX.c   ${fid_src}/tiled_bernsen_threshold.c   ${fid_src}/treeidmap.cpp )
target_include_directories(fidtrack PUBLIC ${fid_src})

add_library(reactivision SHARED
        ${rv_src}/BlobMatrix.cpp
        ${rv_src}/CalibrationEngine.cpp
        ${rv_src}/FidtrackFinder.cpp
        ${rv_src}/FrameThresholder.cpp
        ${rv_src}/BlobObject.cpp
        ${rv_src}/CalibrationGrid.cpp
        ${rv_src}/FiducialFinder.cpp
        ${rv_src}/Settings.cpp )
target_include_directories(reactivision PUBLIC ${rv_src})
target_link_libraries(reactivision portvideo tinyxml tuio osc fidtrack android)

add_library(reactivision-android SHARED AndroidVisionEngine.cpp LogcatInterface.cpp)
target_link_libraries(reactivision-android log reactivision)