#include "nfs_common.h"

int networkfs_create(struct inode *parent_inode, struct dentry *child_dentry, umode_t mode, bool b)
{
	ino_t output_buf;
	struct inode *inode = NULL;
	char *name = name_process(child_dentry->d_name.name);
	char param_name[SRV_MAX_NAME_SIZE];
	char param_parent[SRV_MAX_PARENT_SIZE];
	int ret;

	if (!name) {
		pr_err("cannot allocate buf for name\n");
		goto out;
	}

	snprintf(param_name, SRV_MAX_NAME_SIZE, "%s=%s", SRV_NAME_PARAM, name);
	snprintf(param_parent, SRV_MAX_PARENT_SIZE, "%s=%lu", SRV_PARENT_PARAM, parent_inode->i_ino);

	const char *params[] = {param_parent, param_name, "type=file"};

	ret = connect_to_server(SRV_CREATE_CMD, SRV_CREATE_CMD_CNT, params,
								(const char *) parent_inode->i_sb->s_fs_info, (char *) &output_buf);
	if (ret) {
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);
		goto out;
	}

	inode = networkfs_get_inode(parent_inode->i_sb, NULL, _S_IFREG, output_buf);
	if (!inode) {
		pr_err("cannot get inode\n");
		goto out;
	}

	inode->i_op = &networkfs_inode_ops;
	inode->i_fop = &networkfs_dir_ops;
	d_add(child_dentry, inode);
out:
	kfree(name);
	return ret;
}

int networkfs_unlink(struct inode *parent_inode, struct dentry *child_dentry)
{
	int output_buf;
	char *name = name_process(child_dentry->d_name.name);
	char param_name[SRV_MAX_NAME_SIZE];
	char param_parent[SRV_MAX_PARENT_SIZE];
	int ret;

	if (!name) {
		pr_err("cannot allocate buf for name\n");
		goto out;
	}

	snprintf(param_name, SRV_MAX_NAME_SIZE, "%s=%s", SRV_NAME_PARAM, name);
	snprintf(param_parent, SRV_MAX_PARENT_SIZE, "%s=%lu", SRV_PARENT_PARAM, parent_inode->i_ino);

	const char *params[] = {param_parent, param_name};

	ret = connect_to_server(SRV_UNLINK_CMD, SRV_UNLINK_CMD_CNT, params,
								(const char *) parent_inode->i_sb->s_fs_info, (char *) &output_buf);
	if (ret)
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);

out:
	kfree(name);
	return ret;
}

/* draft */
ssize_t networkfs_read(struct file *filp, char *buffer, size_t len, loff_t *offset)
{
	struct content *output_buf = NULL;
	struct inode *inode = filp->f_inode;
	char param_inode[SRV_MAX_INO_SIZE];
	int ret;
	size_t i, read = 0;

	output_buf = kmalloc(sizeof(struct content), GFP_KERNEL);
	if (!output_buf) {
		pr_err("cannot malloc buffer to recieve message\n");
		goto out;
	}

	snprintf(param_inode, SRV_MAX_INO_SIZE, "%s=%lu", SRV_INO_PARAM, inode->i_ino);

	char const *params[] = {param_inode};

	ret = connect_to_server(SRV_READ_CMD, SRV_READ_CMD_CNT, params,
								(const char *) inode->i_sb->s_fs_info, (char *) &output_buf);
	if (ret)
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);
	ret = 0;

	// ???

	for (i = *offset; i < output_buf->content_length && read < len; i++) {
		put_user(output_buf->content[i], buffer + i);
		read++;
	}

out_buf:
	kfree(output_buf);
out:
	return read;
}