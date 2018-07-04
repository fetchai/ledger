all:
	@echo "This script would delete your root harddrive, but as a favour to" 
	@echo "those who did not install a revertible file system on their computer,"
	@echo "this functionality has been moved."
	@echo
	@echo "For future reference please use:"
	@echo
	@echo "  $ make serious-damage"
	@echo 
	@echo "or just"
	@echo
	@echo "  $ make clean"
	@echo 
	@echo "for short. You are welcome, Ed."
	@echo

clean:
	find . | grep "~" | xargs rm -f
	find . | grep "#" | xargs rm -f
	find . | grep "\.pyc" | xargs rm -f
	find . | grep "\.gch" | xargs rm -f # remove precompiled headers
	find . | grep "\.orig" | xargs rm -f 
	rm -rf build/
#	./adjust_style ./

count:
	find libs/ -type f | grep "\.hpp" | xargs cat | wc -l
	find libs/ -type f | grep "\.cpp" | xargs cat | wc -l



gource:
	gource --seconds-per-day 3 --auto-skip-seconds 1 --camera-mode overview --bloom-multiplier 1.0 --bloom-intensity 1.0 --title "ARL Lib" --highlight-all-users 


prettify:
	./apply_google_style include
	./apply_google_style src
	./apply_google_style examples
	./apply_google_style tests

full_documentation:
	cldoc generate -std=c++11 -DFETCH_COMPILING_DOCUMENTATION -DASIO_STANDALONE -DASIO_HEADER_ONLY -DASIO_HAS_STD_SYSTEM_ERROR -Iinclude -Ivendor/asio/asio/include -I/opt/local/include -- --report --merge docs/src --output docs/html ./include/assert.hpp ./include/byte_array ./include/commandline ./include/crypto ./include/image/ ./include/math ./include/memory ./include/meta ./include/mutex.hpp ./include/network ./include/optimisation ./include/random ./include/service ./include/script ./include/serializer ./include/storage ./include/string ./include/unittest.hpp ./include/math/linalg ./include/math/spline

documentation:
	cldoc generate -std=c++11 -DFETCH_COMPILING_DOCUMENTATION -DASIO_STANDALONE -DASIO_HEADER_ONLY -DASIO_HAS_STD_SYSTEM_ERROR -Iinclude -Ivendor/asio/asio/include -I/opt/local/include -- --report --merge docs/src --output docs/html ./include/service
