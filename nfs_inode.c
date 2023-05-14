#include "nfs_common.h"

struct inode *networkfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, int i_ino)
{
	struct inode *inode;
	inode = new_inode(sb);
	if (!inode)
		return NULL;

	inode->i_ino = i_ino;
	inode->i_op = &networkfs_inode_ops;
	inode->i_fop = &networkfs_dir_ops;
	inode_init_owner(inode, dir, mode);

	return inode;
}

struct dentry* networkfs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flag)
{
	struct entry_info *output_buf = NULL;
	struct inode *inode = NULL;
	char *name = name_process(child_dentry->d_name.name);
	char param_name[SRV_MAX_NAME_SIZE];
	char param_parent[SRV_MAX_PARENT_SIZE];
	int ret;

	if (!name) {
		pr_err("cannot allocate buf for name\n");
		goto out;
	}

	output_buf = kmalloc(sizeof(struct entry_info), GFP_KERNEL);
	if (!output_buf) {
		pr_err("cannot malloc buffer to recieve message\n");
		goto out;
	}

	snprintf(param_name, SRV_MAX_NAME_SIZE, "%s=%s", SRV_NAME_PARAM, name);
	snprintf(param_parent, SRV_MAX_PARENT_SIZE, "%s=%lu", SRV_PARENT_PARAM, parent_inode->i_ino);

	const char *params[] = {param_name, param_parent};

	ret = connect_to_server(SRV_LOOKUP_CMD, SRV_LOOKUP_CMD_CNT, params,
								(const char *) parent_inode->i_sb->s_fs_info, (char *) output_buf);
	if (ret) {
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);
		goto out_buf;
	}

	inode = networkfs_get_inode(parent_inode->i_sb, NULL, ((output_buf->entry_type == DT_DIR) ? _S_IFDIR : _S_IFREG), output_buf->ino);
	if (!inode) {
		pr_err("cannot get inode\n");
		goto out_buf;
	}

	d_add(child_dentry, inode);
out_buf:
	kfree(output_buf);
out:
	kfree(name);
	return NULL;
}

int networkfs_link(struct dentry *old_dentry, struct inode *parent_dir, struct dentry *new_dentry) {
	int output_buf;
	char *name = name_process(new_dentry->d_name.name);
	char param_name[SRV_MAX_NAME_SIZE];
	char param_parent[SRV_MAX_PARENT_SIZE];
	char param_source[SRV_MAX_SOURCE_SIZE];
	int ret;

	if (!name) {
		pr_err("cannot allocate buf for name\n");
		goto out;
	}

	snprintf(param_name, SRV_MAX_NAME_SIZE, "%s=%s", SRV_NAME_PARAM, name);
	snprintf(param_parent, SRV_MAX_PARENT_SIZE, "%s=%lu", SRV_PARENT_PARAM, parent_dir->i_ino);
	snprintf(param_source, SRV_MAX_SOURCE_SIZE, "%s=%lu", SRV_SOURCE_PARAM, old_dentry->d_inode->i_ino);

	const char *params[] = {param_source, param_parent, param_name};

	ret = connect_to_server(SRV_LINK_CMD, SRV_LINK_CMD_CNT, params,
								(const char *) parent_dir->i_sb->s_fs_info, (char *) &output_buf);
	if (ret)
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);

out:
	kfree(name);
	return ret;
}

struct inode_operations networkfs_inode_ops =
{
	.lookup = networkfs_lookup,
	.create = networkfs_create,
	.unlink = networkfs_unlink,
	.mkdir = networkfs_mkdir,
	.rmdir = networkfs_rmdir,
	.link = networkfs_link,
};
