## Build PSU Framework
#rm -rf build
mkdir build
cd build
cmake .. -D PRESET_NAME=linux -D CMAKE_BUILD_TYPE=Release
#cmake .. -D PRESET_NAME=linux -D CMAKE_BUILD_TYPE=Release
make -j 8
