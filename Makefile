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

count:
	find libs/ -type f | grep "\.hpp" | xargs cat | wc -l
	find libs/ -type f | grep "\.cpp" | xargs cat | wc -l
