CFLAGS=-Wall -g

all: ex19 ex18

ex19: object.o

.PHONY: test

test: ex19
	@echo "赋予测试脚本执行权限..."
	chmod +x test.sh
	@echo "运行测试脚本..."
	./test.sh

clean:
	rm -f ex19 object.o
