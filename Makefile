KERNEL_TREE?=/lib/modules/$(shell uname -r)/build
obj-m +=  plugin_nuevo.o

all:
	make -C $(KERNEL_TREE)  M=$(PWD) modules

clean:
	make -C $(KERNEL_TREE)  M=$(PWD) clean
