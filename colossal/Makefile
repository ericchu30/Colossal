all:
	@make -C src
	@make -C test

clean:
	@make -C src clean
	@make -C test clean
	@find . -name "*~" | xargs -I file rm "file"

format:
	@./util/format.sh src/*.hpp src/*.cpp test/*.cpp

.PHONY: all clean format
