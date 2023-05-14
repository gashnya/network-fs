#include "nfs_common.h"

int networkfs_mkdir(struct inode *parent_inode, struct dentry *child_dentry, umode_t mode)
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

	const char *params[] = {param_parent, param_name, "type=directory"};

	ret = connect_to_server(SRV_CREATE_CMD, SRV_CREATE_CMD_CNT, params,
								(const char *) parent_inode->i_sb->s_fs_info, (char *) &output_buf);
	if (ret) {
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);
		goto out;
	}

	inode = networkfs_get_inode(parent_inode->i_sb, NULL, _S_IFDIR, output_buf);
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

int networkfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry)
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

	ret = connect_to_server(SRV_RMDIR_CMD, SRV_RMDIR_CMD_CNT, params,
								(const char *) parent_inode->i_sb->s_fs_info, (char *) &output_buf);
	if (ret)
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);

out:
	kfree(name);
	return ret;
}

int networkfs_iterate(struct file *filp, struct dir_context *ctx)
{
	struct entries *output_buf = NULL;
	struct inode *inode = file_inode(filp);
	char param_inode[SRV_MAX_INO_SIZE];
	int stored = 0;
	int ret;

	output_buf = kmalloc(sizeof(struct entries), GFP_KERNEL);
	if (!output_buf) {
		pr_err("cannot malloc buffer to recieve message\n");
		goto out;
	}

	snprintf(param_inode, SRV_MAX_INO_SIZE, "%s=%lu", SRV_INO_PARAM, inode->i_ino);

	const char *params[] = {param_inode};

	ret = connect_to_server(SRV_ITERATE_CMD, SRV_ITERATE_CMD_CNT, params,
								(const char *) inode->i_sb->s_fs_info, (char *) output_buf);
	if (ret) {
		pr_err("non-zero ret while trying to connect to the server: %d\n", ret);
		goto out_buf;
	}

	if (!dir_emit_dots(filp, ctx))
		goto out_buf;
	stored = ctx->pos;

	while (stored - 2 < output_buf->entries_count) {
		struct entry file = output_buf->entries[stored - 2];

		if (!dir_emit(ctx, file.name, strlen(file.name), file.ino, file.entry_type)) {
			goto out_buf;
		}
		stored++;
		ctx->pos++;
	}

out_buf:
	kfree(output_buf);
out:
	return stored;
}

struct file_operations networkfs_dir_ops =
{
	.iterate = networkfs_iterate,
};
