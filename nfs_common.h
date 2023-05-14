#ifndef NFS_COMMON_H
#define NFS_COMMON_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/inet.h>
#include <linux/in.h>
#include <net/sock.h>

/* utils */

#define ESOCKNOCREATE  -1
#define ESOCKNOCONNECT -2
#define ESOCKNOMSGSEND -3
#define ESOCKNOMSGRECV -4
#define ENOOKRESPONSE  -5
#define ENOSPACE       -10

#define IS_DIGIT(c) (c >= 48 && c <= 57)
#define IS_ALPH(c) ((c >= 64 && c <= 90) || (c >= 97 && c <= 122))
/* underscore, full stop, hyphen-minus, tilde */
#define IS_OTHER_RES_SYM(c) (c == 45 || c == 46 || c == 95 || c == 126)

#define _S_IFDIR (S_IFDIR | S_IRWXUGO)
#define _S_IFREG (S_IFREG | S_IRWXUGO)

/* params for the server */

#define SRV_NAME_PARAM "name"
#define SRV_PARENT_PARAM "parent"
#define SRV_INO_PARAM "inode"
#define SRV_SOURCE_PARAM "source"

#define SRV_MAX_NAME_LEN 255

#define SRV_MAX_NAME_SIZE (sizeof(SRV_NAME_PARAM) + SRV_MAX_NAME_LEN * 3 + 2)
#define SRV_MAX_PARENT_SIZE (sizeof(SRV_PARENT_PARAM) + sizeof(unsigned long) + 2)
#define SRV_MAX_INO_SIZE (sizeof(SRV_INO_PARAM) + sizeof(unsigned long) + 2)
#define SRV_MAX_SOURCE_SIZE (sizeof(SRV_SOURCE_PARAM) + sizeof(unsigned long) + 2)

/* server commands */

#define SRV_LOOKUP_CMD "lookup"
#define SRV_ITERATE_CMD "list"
#define SRV_CREATE_CMD "create"
#define SRV_UNLINK_CMD "unlink"
#define SRV_RMDIR_CMD "rmdir"
#define SRV_LINK_CMD "link"
#define SRV_READ_CMD "read"

#define SRV_LOOKUP_CMD_CNT 2
#define SRV_ITERATE_CMD_CNT 1
#define SRV_CREATE_CMD_CNT 3
#define SRV_UNLINK_CMD_CNT 2
#define SRV_RMDIR_CMD_CNT 2
#define SRV_LINK_CMD_CNT 3
#define SRV_READ_CMD_CNT 1

/* buf size for answer from the server */

#define LOOKUP_OUTBUF_SIZE sizeof(struct entry_info)
#define ITERATE_OUTBUF_SIZE sizeof(struct entries)

#define ROOT_INO 1000

/* structs of the answers from the server */

struct entry_info {
	unsigned char entry_type;
	ino_t ino;
};

struct entry {
	unsigned char entry_type;
	ino_t ino;
	char name[256];
};

struct entries {
	size_t entries_count;
	struct entry entries[16];
};

struct content {
	u64 content_length;
	char content[];
};

extern struct file_operations networkfs_dir_ops;
extern struct inode_operations networkfs_inode_ops;

/* utils */

char *name_process(const unsigned char *name);
int connect_to_server(const char *command, int params_count, const char *params[], const char *token, char *output_buf);
/* static? */
int send_msg_to_server(struct socket *sock, char *send_buf);
int recv_msg_from_server(struct socket *sock, char *recv_buf, int recv_buf_size);
int connect_to_server_atoi(const char *c);

/* file */

int networkfs_create(struct inode *parent_inode, struct dentry *child_dentry, umode_t mode, bool b);
int networkfs_unlink(struct inode *parent_inode, struct dentry *child_dentry);

/* dir */

int networkfs_mkdir(struct inode *parent_inode, struct dentry *child_dentry, umode_t mode);
int networkfs_rmdir(struct inode *parent_inode, struct dentry *child_dentry);
int networkfs_iterate(struct file *filp, struct dir_context *ctx);

/* inode */

struct inode *networkfs_get_inode(struct super_block *sb, const struct inode *dir, umode_t mode, int i_ino);
struct dentry* networkfs_lookup(struct inode *parent_inode, struct dentry *child_dentry, unsigned int flag);
int networkfs_link(struct dentry *old_dentry, struct inode *parent_dir, struct dentry *new_dentry);

#endif
