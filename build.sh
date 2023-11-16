if [ ! -d build ]; then
	mkdir build
	cd build
	cmake ..
	cd ..
fi
cd build
cmake --build .