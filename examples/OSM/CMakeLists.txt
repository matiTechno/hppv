add_executable(OSM OSM.cpp)
target_link_libraries(OSM hppv)
file(COPY map.osm DESTINATION .)
