
clean:
	find . | grep "~" | xargs rm -f
	find . | grep "#" | xargs rm -f
	find . | grep "\.pyc" | xargs rm -f
	find . | grep "\.gch" | xargs rm -f # remove precompiled headers
	find . | grep "\.orig" | xargs rm -f 
	rm -rf build/
#	./adjust_style ./

count:
	find include/ -type f | grep "\.hpp" | xargs cat | wc -l
	find tests/ -type f | grep "\.cpp" | xargs cat | wc -l



gource:
	gource --seconds-per-day 3 --auto-skip-seconds 1 --camera-mode overview --bloom-multiplier 1.0 --bloom-intensity 1.0 --title "ARL Lib" --highlight-all-users 


prettify:
	./apply_google_style include
	./apply_google_style src
	./apply_google_style examples
	./apply_google_style tests
