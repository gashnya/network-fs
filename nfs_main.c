#include "nfs_common.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bodrikova Jevgenia");
MODULE_VERSION("0.01");

struct file_system_type networkfs_fs_type;

static void networkfs_kill_sb(struct super_block *sb)
{
	kfree(sb->s_fs_info);
	pr_info("networkfs super block is destroyed\n");
}

static int networkfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *inode = networkfs_get_inode(sb, NULL, _S_IFDIR, ROOT_INO);

	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
}

static struct dentry* networkfs_mount(struct file_system_type *fs_type, int flags, const char *token, void *data)
{
	struct dentry *dentry = mount_nodev(fs_type, flags, data, networkfs_fill_super);
	char* _token = NULL;

	if (!dentry) {
		pr_err("cannot mount networkfs\n");
		return NULL;
	}

	_token = kmalloc(strlen(token), GFP_KERNEL);
	if (!token) {
		pr_err("cannot allocate buf for token\n");
		return NULL;
	}

	strcpy(_token, token);
	dentry->d_sb->s_fs_info = _token;
	pr_info("networkfs mounted successfuly\n");
	return dentry;
}

static int __init networkfs_init(void)
{
	int ret;

	ret = register_filesystem(&networkfs_fs_type);
	if (!ret)
		pr_info("networkfs registered");
	else
		pr_err("cannot register networkfs");
	return ret;
}

static void __exit networkfs_exit(void)
{
	unregister_filesystem(&networkfs_fs_type);
	pr_info("networkfs unregistered\n");
}

struct file_system_type networkfs_fs_type =
{
	.name = "networkfs",
	.mount = networkfs_mount,
	.kill_sb = networkfs_kill_sb,
};

module_init(networkfs_init);
module_exit(networkfs_exit);
