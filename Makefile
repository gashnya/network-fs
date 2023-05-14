obj-m += networkfs.o
networkfs-y := nfs_main.o nfs_dir.o nfs_file.o nfs_inode.o nfs_utils.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

