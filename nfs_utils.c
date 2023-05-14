#ifndef LAB_UTILS_C
#define LAB_UTILS_C

#include "nfs_common.h"

/**
 *	send_msg_to_server - send a @send_buf through @sock (kernel-space)
 *	@sock: socket
 *  @send_buf: buffer to send
 *  ------------------------------------------------------------------
 *  kernel_sendmsg
 *	@msg: message header
 *	@vec: kernel vec
 *	@num: vec array length
 *	@size: total message data size
 *
 *	Creates @vec and @msg from @send_buf.
 *	Builds the message data with @vec and sends it through @sock.
 *	Returns the number of bytes sent, or an error code.
 */

char *name_process(const unsigned char *name)
{
	/* % + .. */
	char *name_processed = kmalloc(SRV_MAX_NAME_LEN * 3 + 1, GFP_KERNEL);
	int i, p_i;

	if (!name_processed)
		return NULL;

	for (i = 0, p_i = 0; i < strlen(name); i++, p_i++) {
		int c = (int) name[i];
		if (IS_DIGIT(c) || IS_ALPH(c) || IS_OTHER_RES_SYM(c)) {
			sprintf(name_processed + p_i, "%c", name[i]);
		} else {
			sprintf(name_processed + p_i, "%%%x", name[i]);
			p_i += 2;
		}
	}

	pr_warn("%s     %s\n", name, name_processed);
	return name_processed;
}

int send_msg_to_server(struct socket *sock, char *send_buf)
{
	struct kvec send_vec;
	struct msghdr send_msg;
	memset(&send_msg, 0, sizeof(send_msg));
	memset(&send_vec, 0, sizeof(send_vec));
	send_vec.iov_base = send_buf;
	send_vec.iov_len = strlen(send_buf);
	return kernel_sendmsg(sock, &send_msg, &send_vec, 1, strlen(send_buf));
}

char connect_to_server_output_buf[8192];
const int connect_to_server_output_buf_size = 8192;

/**
 *	recv_msg_from_server - Receive a message from a socket (kernel space)
 *	@sock: The socket to receive the message from
 *  @recv_buf: Buffer to fill
 *  @recv_buf: Size of @recv_buf
 *  ---------------------------------------------------------------------
 *  kernel_recvmsg
 *	@msg: Received message
 *	@vec: Input s/g array for message data
 *	@num: Size of input s/g array
 *	@size: Number of bytes to read
 *	@flags: Message flags (MSG_DONTWAIT, etc...)
 *
 *  Creates @vec and @msg.
 *	On return the msg structure contains the scatter/gather array passed in the
 *	vec argument. The array is modified so that it consists of the unfilled
 *	portion of the original array.
 *
 *	The returned value is the total number of bytes received, or an error.
 */

int recv_msg_from_server(struct socket *sock, char *recv_buf, int recv_buf_size)
{
	struct kvec recv_vec;
	struct msghdr recv_msg;
	int ret, sum;
	sum = 0;
	memset(recv_buf, 0, recv_buf_size);
	while (true)
	{
		memset(&recv_msg, 0, sizeof(recv_msg));
		memset(&recv_vec, 0, sizeof(recv_vec));
		recv_vec.iov_base = connect_to_server_output_buf;
		recv_vec.iov_len = connect_to_server_output_buf_size;
		ret = kernel_recvmsg(sock, &recv_msg, &recv_vec, 1, connect_to_server_output_buf_size, 0);
		if (ret == 0)
		{
			break;
		}
		else if (ret < 0)
		{
			return ESOCKNOMSGRECV;
		}
		memcpy(recv_buf + sum, connect_to_server_output_buf, ret);
		sum += ret;
	}
	return sum;
}

/* just atoi */
int connect_to_server_atoi(const char *c)
{
	int res;
	res = 0;
	while (*c != '\n' && *c != '\r')
	{
		int cur_c = *c - '0';
		if (cur_c < 0 || cur_c > 9)
		{
			return -1;
		}
		res = 10 * res + cur_c;
		c++;
	}
	return res;
}

char connect_to_server_send_buf[8192];
char connect_to_server_recv_buf[8192];

/**
 *	connect_to_server - HTTP-client
 *	@command: name of the method (list, create, read, write, lookup, unlink, rmdir, link)
 *  @params_count: number of parameters
 *  @params: list of parameters (<parameter_name>=<value>)
 *  @token: token
 *	@output_buf: buffer to fill with answer from server (size at least 8192 bytes)
 *
 *	Returns 0 on success or an error (utils.c:73)
 */

int connect_to_server(const char *command, int params_count, const char *params[], const char *token, char *output_buf)
{
	const int BUFFER_SIZE = 8192;
	struct socket sock;
	struct socket *sock_ptr;
	struct sockaddr_in s_addr;
	char *send_buf, *recv_buf;
	int i;
	int error;
	int message_len;
	send_buf = connect_to_server_send_buf;
	recv_buf = connect_to_server_recv_buf;
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(80);
	s_addr.sin_addr.s_addr = in_aton("77.234.215.132");
	sock_ptr = &sock;
	error = sock_create_kern(&init_net, AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock_ptr);
	if (error < 0)
	{
		return ESOCKNOCREATE;
	}
	error = sock_ptr->ops->connect(sock_ptr, (struct sockaddr *)&s_addr, sizeof(s_addr), 0);
	if (error != 0)
	{
		return ESOCKNOCONNECT;
	}
	strcpy(send_buf, "GET /teaching/os/networkfs/v1/");
	strcat(send_buf, token);
	strcat(send_buf, "/fs/");
	strcat(send_buf, command);
	strcat(send_buf, "?");
	i = 0;
	while (i < params_count)
	{
		if (i != 0)
		{
			strcat(send_buf, "&");
		}
		strcat(send_buf, params[i]);
		i++;
	}
	strcat(send_buf, " HTTP/1.1\r\nHost: nerc.itmo.ru\r\nConnection: close\r\n\r\n");
	error = send_msg_to_server(sock_ptr, send_buf);
	if (error < 0)
	{
		return ESOCKNOMSGSEND;
	}
	error = recv_msg_from_server(sock_ptr, recv_buf, BUFFER_SIZE);
	if (error < 0)
	{
		return ESOCKNOMSGRECV;
	}
	if (strncmp(recv_buf, "HTTP/1.1 200 ", strlen("HTTP/1.1 200 ")) != 0)
	{
		return ENOOKRESPONSE;
	}
	recv_buf = strchr(recv_buf, '\n') + 1;
	recv_buf = strchr(recv_buf, '\n') + 1;
	recv_buf = strchr(recv_buf, '\n') + 1;
	recv_buf = strchr(recv_buf, '\n') + 1;
	recv_buf = strchr(recv_buf, ' ') + 1;
	message_len = connect_to_server_atoi(recv_buf);
	recv_buf = strchr(recv_buf, '\n') + 1;
	recv_buf = strchr(recv_buf, '\n') + 1;
	recv_buf = strchr(recv_buf, '\n') + 1;
	error = *(int *)(recv_buf);
	recv_buf += 8;
	memcpy(output_buf, recv_buf, message_len);
	kernel_sock_shutdown(sock_ptr, SHUT_RDWR);
	sock_release(sock_ptr);
	return error;
}

#endif
