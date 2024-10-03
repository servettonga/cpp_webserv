# External Functions

### `execve` - execute program

Synopsis:

```C
#include <unistd.h>

int execve(const char *pathname, char *const _Nullable argv[], char *const _Nullable envp[]);
```

Description:

	execve() executes the program referred to by pathname. This causes the program that is currently being run
	by the calling process to be replaced with a new program, with newly initialized stack, heap, and (initial‐
	ized and uninitialized) data segments.

Return:

	On success, execve() does not return, on error -1 is returned, and errno is set to indicate the error.
---

### `dup`, `dup2` - duplicate a file descriptor

Synopsis:

```C
#include <unistd.h>

int dup(int oldfd);
int dup2(int oldfd, int newfd);
```

Description:

	The dup() system call allocates a new file descriptor that refers to the same open file description as the descriptor oldfd. (For an explanation of open file descriptions, see open(2).) The new file descriptor number
	is guaranteed to be the lowest-numbered file descriptor that was unused in the calling process.

	After a successful return, the old and new file descriptors may be used interchangeably.

	dup2()
	The dup2() system call performs the same task as dup(), but instead of using the lowest-numbered unused file descriptor, it uses the file descriptor number specified in newfd. In other words, the file descriptor newfd is adjusted so that it now refers to the same open file description as oldfd.

Return:

	On success, these system calls return the new file descriptor. On error, -1 is returned, and errno is set to indicate the error.
---

### `pipe` - create pipe

Synopsis:

```C
#include <unistd.h>

int pipe(int pipefd[2]);
```

Description:

	pipe()  creates a pipe, a unidirectional data channel that can be used for interprocess communication.
	The	array pipefd is used to return two file descriptors referring to the ends of the pipe.  pipefd[0] refers to
	the read end of the pipe.  pipefd[1] refers to the write end of the pipe.  Data written to the write end of	the pipe is buffered by the kernel until it is read from the read end of the pipe.   For  further  details,	see pipe(7).

Return:

	On  success, zero is returned.  On error, -1 is returned, errno is set to indicate the error, and pipefd is left unchanged.
---

### `strerror` - return string describing error number

Synopsis:

```C
#include <string.h>

char *strerror(int errnum);
```

Description:

	The  strerror() function returns a pointer to a string that describes the error code passed in the argument
	errnum, possibly using the LC_MESSAGES part of the current locale to select the appropriate language.  (For
	example, if errnum is EINVAL, the returned description will be "Invalid argument".)  This string  must  not
	be modified by the application, but may be modified by a subsequent call to strerror() or strerror_l().  No
	other library function, including perror(3), will modify this string.

Return:

	The strerror(), strerror_l(), and the GNU‐specific strerror_r() functions return the appropriate error
	description string, or an "Unknown error nnn" message if the error number is unknown.

	On  success,  strerrorname_np()  and strerrordesc_np() return the appropriate error description string.  If
	errnum is an invalid error number, these functions return NULL.
---

### `getaddrinfo`, `freeaddrinfo`, `gai_strerror` - network address and service translation

Synopsis:

```C
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int getaddrinfo(const char *restrict node,
				const char *restrict service,
				const struct addrinfo *restrict hints,
				struct addrinfo **restrict res);

void freeaddrinfo(struct addrinfo *res);

const char *gai_strerror(int errcode);
```

Description:

	Given  node  and  service, which identify an Internet host and a service, getaddrinfo() returns one or more
	addrinfo structures, each of which contains an Internet address that can be specified in a call to  bind(2)
	or  connect(2).  The getaddrinfo() function combines the functionality provided by the gethostbyname(3) and
	getservbyname(3) functions into a single interface, but unlike the latter functions, getaddrinfo() is reen‐
	trant and allows programs to eliminate IPv4‐versus‐IPv6 dependencies.

	The addrinfo structure used by getaddrinfo() contains the following fields:

		struct addrinfo {
			int              ai_flags;
			int              ai_family;
			int              ai_socktype;
			int              ai_protocol;
			socklen_t        ai_addrlen;
			struct sockaddr *ai_addr;
			char            *ai_canonname;
			struct addrinfo *ai_next;
		};

Return:

	getaddrinfo() returns 0 if it succeeds, or one of the following nonzero error codes:
	...
	The gai_strerror() function translates these error codes to a human readable string, suitable for error re‐
	porting.

---

### `errno` - number of last error

Synopsis:

```C
#include <errno.h>
```

Description:

	The <errno.h> header file defines the integer variable errno, which is set by system calls and some library
	functions in the event of an error to indicate what went wrong.
---

### `fork` - create a child process

Synopsis:

```C
#include <unistd.h>

pid_t fork(void);
```

Description:

	fork()  creates  a  new  process by duplicating the calling process.  The new process is referred to as the
	child process.  The calling process is referred to as the parent process.

Return:

	On success, the PID of the child process is returned in the parent, and 0 is returned  in  the  child.
	On failure,	-1 is returned in the parent, no child process is created, and errno is set to indicate the error.
---

### `socketpair` - create a pair of connected sockets

Synopsis:

```C
#include <sys/socket.h>

int socketpair(int domain, int type, int protocol, int sv[2]);
```

Description:

	The  socketpair()  call creates an unnamed pair of connected sockets in the specified domain, of the speci‐
	fied type, and using the optionally specified protocol.   For  further  details  of  these  arguments,  see
	socket(2).

Return:

	On success, zero is returned.  On error, -1 is returned, errno is set to indicate the error, and sv is
	left unchanged.
---

### `htonl`, `htons`, `ntohl`, `ntohs` - convert values between host and network byte order

Synopsis:

```C
#include <arpa/inet.h>

uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);

uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);
```

Description:

       The htonl() function converts the unsigned integer hostlong from host byte order to network byte order.

       The htons() function converts the unsigned short integer hostshort from host byte order to network byte order.

       The ntohl() function converts the unsigned integer netlong from network byte order to host byte order.

       The  ntohs() function converts the unsigned short integer netshort from network byte order to host byte order.

---

### `select` - synchronous I/O multiplexing

Synopsis:

```C
#include <sys/select.h>

typedef /* ... */ fd_set;

int select(int nfds, fd_set *_Nullable restrict readfds,
			fd_set *_Nullable restrict writefds,
			fd_set *_Nullable restrict exceptfds,
			struct timeval *_Nullable restrict timeout);

void FD_CLR(int fd, fd_set *set);
int  FD_ISSET(int fd, fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);
```

Description:

	select()  allows  a program to monitor multiple file descriptors, waiting until one or more of the file
	descriptors become "ready" for some class of I/O operation (e.g., input possible).  A file descriptor is
	considered ready if it is possible to perform a corresponding I/O operation (e.g., read(2), or a sufficiently
	small write(2)) without blocking.

	WARNING:  select() can monitor only file descriptors numbers that are less than FD_SETSIZE (1024)—an unrea‐
	sonably low limit for many modern applications—and this limitation will not change.   All  modern  applica‐
	tions should instead use poll(2) or epoll(7), which do not suffer this limitation.

Return:

	On success, select() and pselect() return the number of file descriptors contained in  the  three  returned
	descriptor  sets (that is, the total number of bits that are set in readfds, writefds, exceptfds).  The re‐
	turn value may be zero if the timeout expired before any file descriptors became ready.

	On error, -1 is returned, and errno is set to indicate the error; the file descriptor sets are  unmodified,
	and timeout becomes undefined.
---

### `poll` - wait for some event on a file descriptor

Synopsis:

```C
#include <poll.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```

Description:

	poll()  performs a similar task to select(2): it waits for one of a set of file descriptors to become ready
	to perform I/O.  The Linux‐specific epoll(7) API performs a similar task, but offers features beyond  those
	found in poll().

	The  set  of file descriptors to be monitored is specified in the fds argument, which is an array of struc‐
	tures of the following form:

		struct pollfd {
			int   fd;         /* file descriptor */
			short events;     /* requested events */
			short revents;    /* returned events */
		};

	The caller should specify the number of items in the fds array in nfds.

Return:

	On success, poll() returns a nonnegative value which is the number of elements in the pollfds whose revents
	fields  have  been  set to a nonzero value (indicating an event or an error).  A return value of zero indi‐
	cates that the system call timed out before any file descriptors became ready.

	On error, -1 is returned, and errno is set to indicate the error.
---

### `epoll` - I/O event notification facility

Synopsis:

```C
#include <sys/epoll.h>
```

Description:

	The  epoll  API  performs  a similar task to poll(2): monitoring multiple file descriptors to see if I/O is
	possible on any of them.  The epoll API can be used either as an edge‐triggered or a level‐triggered inter‐
	face and scales well to large numbers of watched file descriptors.

---

### `epoll_create` - open an epoll file descriptor

Synopsis:

```C
#include <sys/epoll.h>

int epoll_create(int size);
```

Description:

	epoll_create()  creates a new epoll(7) instance.  Since Linux 2.6.8, the size argument is ignored, but must
	be greater than zero; see NOTES.

	epoll_create() returns a file descriptor referring to the new epoll instance.  This file descriptor is used
	for all the subsequent calls to the epoll interface.  When no longer required, the file descriptor returned
	by epoll_create() should be closed by using close(2).  When all file descriptors referring to an epoll  in‐
	stance have been closed, the kernel destroys the instance and releases the associated resources for reuse.

Return:

	On success, these system calls return a file descriptor (a nonnegative integer).  On error, -1 is returned,
	and errno is set to indicate the error.
---

### `epoll_ctl` - control interface for an epoll file descriptor

Synopsis:

```C
#include <sys/epoll.h>

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *_Nullable event);
```

Description:

	This  system  call  is used to add, modify, or remove entries in the interest list of the epoll(7) instance
	referred to by the file descriptor epfd.  It requests that the operation op be  performed  for  the  target
	file descriptor, fd.

Return:

	When successful, epoll_ctl() returns zero.  When an error occurs, epoll_ctl() returns -1 and errno  is  set
	to indicate the error.
---

### `epoll_wait` - wait for an I/O event on an epoll file descriptor

Synopsis:

```C
#include <sys/epoll.h>

int epoll_wait(int epfd, struct epoll_event *events,
				int maxevents, int timeout);
```

Description:

	The  epoll_wait()  system call waits for events on the epoll(7) instance referred to by the file descriptor
	epfd.  The buffer pointed to by events is used to return information from the ready  list  about  file  de‐
	scriptors  in  the  interest  list  that  have  some  events  available.   Up  to maxevents are returned by
	epoll_wait().  The maxevents argument must be greater than zero.

Return:

	On success, epoll_wait() returns the number of file descriptors ready for the requested I/O, or zero if  no
	file  descriptor  became ready during the requested timeout milliseconds.  On failure, epoll_wait() returns
	-1 and errno is set to indicate the error.
---

### `kqueue`, `kevent` - kernel	event notification mechanism

Synopsis:

```C
#include <sys/event.h>

int kqueue(void);

int kevent(int kq, const struct	kevent *changelist, int nchanges,
			struct kevent *eventlist, int nevents, const struct timespec *timeout);

EV_SET(kev, ident, filter, flags, fflags, data, udata);
```

Description:

	The  kqueue()  system  call  provides a generic method of notifying the
	user when an event happens or a condition holds,	based on  the  results
	of  small pieces	of kernel code termed filters.	A kevent is identified
	by the (ident, filter) pair; there may only be one  unique  kevent  per
	kqueue.

Return:

	The kqueue() system call	creates	a new kernel event queue and returns a
	file  descriptor.   If  there  was  an  error creating the kernel event
	queue, a	value of -1 is returned	and errno set.
---

### `socket` - create an endpoint for communication

Synopsis:

```C
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
```

Description:

	socket()  creates an endpoint for communication and returns a file descriptor that refers to that endpoint.
	The file descriptor returned by a successful call will be the lowest‐numbered file descriptor not currently
	open for the process.

Return:

	On success, a file descriptor for the new socket is returned.  On error, -1 is returned, and errno  is  set
	to indicate the error.
---

### `accept` - accept a connection on a socket

Synopsis:

```C
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *_Nullable restrict addr,
			socklen_t *_Nullable restrict addrlen);
```

Description:

	The  accept() system call is used with connection‐based socket types (SOCK_STREAM, SOCK_SEQPACKET).  It ex‐
	tracts the first connection request on the queue of pending connections for the listening  socket,  sockfd,
	creates a new connected socket, and returns a new file descriptor referring to that socket.  The newly cre‐
	ated socket is not in the listening state.  The original socket sockfd is unaffected by this call.

Return:

	On  success,  these  system calls return a file descriptor for the accepted socket (a nonnegative integer).
	On error, -1 is returned, errno is set to indicate the error, and addrlen is left unchanged.
---

### `listen` - listen for connections on a socket

Synopsis:

```C
#include <sys/socket.h>

int listen(int sockfd, int backlog);
```

Description:

	listen() marks the socket referred to by sockfd as a passive socket, that is, as a socket that will be used
	to accept incoming connection requests using accept(2).

Return:

	On success, zero is returned.  On error, -1 is returned, and errno is set to indicate the error.
---

### `send` - send a message on a socket

Synopsis:

```C
#include <sys/socket.h>

ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
```

Description:

	The system calls send(), sendto(), and sendmsg() are used to transmit a message to another socket.

	The send() call may be used only when the socket is in a connected state (so that the intended recipient is
	known).  The only difference between send() and write(2) is the presence of flags.  With a zero flags argu‐
	ment, send() is equivalent to write(2).  Also, the following call

		send(sockfd, buf, len, flags);

	is equivalent to

		sendto(sockfd, buf, len, flags, NULL, 0);

	The argument sockfd is the file descriptor of the sending socket.

Return:

	On success, these calls return the number of bytes sent.  On error, -1 is returned, and errno is set to in‐
	dicate the error.
---

### `recv` - receive a message from a socket

Synopsis:

```C
#include <sys/socket.h>

ssize_t recv(int sockfd, void buf[.len], size_t len,
				int flags);
```

Description:

	The  recv(),  recvfrom(), and recvmsg() calls are used to receive messages from a socket.  They may be used
	to receive data on both connectionless and connection‐oriented sockets.  This page first  describes  common
	features of all three system calls, and then describes the differences between the calls.

Return:

	These calls return the number of bytes received, or -1 if an error occurred.  In the event of an error, er‐
	rno is set to indicate the error.

	When a stream socket peer has performed an orderly shutdown, the return value will be  0  (the  traditional
	"end‐of‐file" return).
---

### `chdir` - change working directory

Synopsis:

```C
#include <unistd.h>

int chdir(const char *path);
```

Description:

	chdir() changes the current working directory of the calling process to the directory specified in path.

Return:

	On success, zero is returned.  On error, -1 is returned, and errno is set to indicate the error.
---

### `bind` - bind a name to a socket

Synopsis:

```C
#include <sys/socket.h>

int bind(int sockfd, const struct sockaddr *addr,
		socklen_t addrlen);
```

Description:

	When  a socket is created with socket(2), it exists in a name space (address family) but has no address as‐
	signed to it.  bind() assigns the address specified by addr to the socket referred to by the file  descrip‐
	tor sockfd.  addrlen specifies the size, in bytes, of the address structure pointed to by addr.  Tradition‐
	ally, this operation is called “assigning a name to a socket”.

Return:

       On success, zero is returned.  On error, -1 is returned, and errno is set to indicate the error.
---

### `connect` - initiate a connection on a socket

Synopsis:

```C
#include <sys/socket.h>

int connect(int sockfd, const struct sockaddr *addr,
			socklen_t addrlen);
```

Description:

	The  connect()  system  call  connects  the socket referred to by the file descriptor sockfd to the address
	specified by addr.  The addrlen argument specifies the size of addr.  The format of the address in addr  is
	determined by the address space of the socket sockfd; see socket(2) for further details.

Return:

	If the connection or binding succeeds, zero is returned.  On error, -1 is returned, and errno is set to in‐
	dicate the error.
---

### `setsockopt` - get and set options on sockets

Synopsis:

```C
#include <sys/socket.h>

int setsockopt(int sockfd, int level, int optname,
				const void optval[.optlen],
				socklen_t optlen);
```

Description:

	getsockopt()  and setsockopt() manipulate options for the socket referred to by the file descriptor sockfd.
	Options may exist at multiple protocol levels; they are always present at the uppermost socket level.

Return:

	On success, zero is returned for the standard options.  On error, -1 is returned, and errno is set to indi‐
	cate the error.
---

### `getsockname` - get socket name

Synopsis:

```C
#include <sys/socket.h>

int getsockname(int sockfd, struct sockaddr *restrict addr,
				socklen_t *restrict addrlen);
```

Description:

	getsockname()  returns the current address to which the socket sockfd is bound, in the buffer pointed to by
	addr.  The addrlen argument should be initialized to indicate the amount of space (in bytes) pointed to  by
	addr.  On return it contains the actual size of the socket address.

	The  returned address is truncated if the buffer provided is too small; in this case, addrlen will return a
	value greater than was supplied to the call.

Return:

	On success, zero is returned.  On error, -1 is returned, and errno is set to indicate the error.
---

### `getprotobyname` - get protocol entry

Synopsis:

```C
#include <netdb.h>

struct protoent *getprotobyname(const char *name);
```

Description:

	The getprotobyname() function returns a protoent structure for the entry from the database that matches the
	protocol name name.  A connection is opened to the database if necessary.

Return:

	The getprotoent(), getprotobyname(), and getprotobynumber() functions return a pointer to a statically  al‐
	located protoent structure, or a null pointer if an error occurs or the end of the file is reached.
---

### `fcntl` - manipulate file descriptor

Synopsis:

```C
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /* arg */ );
```

Description:

	fcntl()  performs  one  of the operations described below on the open file descriptor fd.  The operation is
	determined by cmd.

	fcntl() can take an optional third argument.  Whether or not this argument is  required  is  determined  by
	cmd.   The  required  argument type is indicated in parentheses after each cmd name (in most cases, the re‐
	quired type is int, and we identify the argument using the name arg), or void is specified if the  argument
	is not required.

Return:

	On error, -1 is returned, and errno is set to indicate the error.
	For a successful call, the return value depends on the operation:
	...
---

### `close` - close a file descriptor

Synopsis:

```C
#include <unistd.h>

int close(int fd);
```

Description:

	close()  closes  a  file descriptor, so that it no longer refers to any file and may be reused.  Any record
	locks (see fcntl(2)) held on the file it was associated with, and owned by the process,  are  removed  (re‐
	gardless of the file descriptor that was used to obtain the lock).

Return:

	close() returns zero on success.  On error, -1 is returned, and errno is set to indicate the error.
---

### `read` - close a file descriptor

Synopsis:

```C
#include <unistd.h>

ssize_t read(int fd, void buf[.count], size_t count);
```

Description:

	read() attempts to read up to count bytes from file descriptor fd into the buffer starting at buf.

Return:

	On success, the number of bytes read is returned (zero indicates end of file), and the file position is ad‐
	vanced by this number.  It is not an error if this number is smaller than the number  of  bytes  requested;
	this  may  happen  for  example because fewer bytes are actually available right now (maybe because we were
	close to end‐of‐file, or because we are reading from a pipe, or from a terminal), or because read() was in‐
	terrupted by a signal.  See also NOTES.

	On error, -1 is returned, and errno is set to indicate the error.  In this case,  it  is  left  unspecified
	whether the file position (if any) changes.
---

### `write` - write to a file descriptor

Synopsis:

```C
#include <unistd.h>

ssize_t write(int fd, const void buf[.count], size_t count);
```

Description:

	write()  writes  up  to count bytes from the buffer starting at buf to the file referred to by the file de‐
	scriptor fd.

Return:

	On  success,  the number of bytes written is returned.  On error, -1 is returned, and errno is set to indi‐
	cate the error.
---

### `waitpid` - wait for process to change state

Synopsis:

```C
#include <sys/wait.h>

pid_t waitpid(pid_t pid, int *_Nullable wstatus, int options);
```

Description:

	The waitpid() system call suspends execution of the calling thread until a child specified by pid  argument
	has  changed state.  By default, waitpid() waits only for terminated children, but this behavior is modifi‐
	able via the options argument, as described below.
	...

Return:

	waitpid(): on success, returns the process ID of the child whose state has changed; if WNOHANG  was  speci‐
	fied and one or more child(ren) specified by pid exist, but have not yet changed state, then 0 is returned.
	On failure, -1 is returned.
---

### `kill` - send signal to a process

Synopsis:

```C
#include <signal.h>

int kill(pid_t pid, int sig);
```

Description:

	The kill() system call can be used to send any signal to any process group or process.

Return:

	On success (at least one signal was sent), zero is returned.  On error, -1 is returned, and errno is set to
	indicate the error.
---

### `signal` - ANSI C signal handling

Synopsis:

```C
#include <signal.h>

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);
```

Description:

	signal() sets the disposition of the signal signum to handler, which is either SIG_IGN, SIG_DFL, or the ad‐
	dress of a programmer‐defined function (a "signal handler").

Return:

	signal() returns the previous value of the signal handler.  On failure, it returns SIG_ERR,  and  errno  is
	set to indicate the error.
---

### `access` - check user’s permissions for a file

Synopsis:

```C
#include <unistd.h>

int access(const char *pathname, int mode);
```

Description:

	access()  checks whether the calling process can access the file pathname.  If pathname is a symbolic link,
	it is dereferenced.

Return:

	On success (all requested permissions granted, or mode is F_OK and the file exists), zero is returned.   On
	error  (at  least  one bit in mode asked for a permission that is denied, or mode is F_OK and the file does
	not exist, or some other error occurred), -1 is returned, and errno is set to indicate the error.
---

### `stat` - get file status

Synopsis:

```C
#include <sys/stat.h>

int stat(const char *restrict pathname, struct stat *restrict statbuf);
```

Description:

	stat()  and  fstatat()  retrieve information about the file pointed to by pathname; the differences for fs‐
	tatat() are described below.

Return:

	On success, zero is returned.  On error, -1 is returned, and errno is set to indicate the error.
---

### `opendir` - open a directory

Synopsis:

```C
#include <sys/types.h>
#include <dirent.h>

DIR *opendir(const char *name);
```

Description:

	The  opendir() function opens a directory stream corresponding to the directory name, and returns a pointer
	to the directory stream.  The stream is positioned at the first entry in the directory.

Return:

	The  opendir()  and  fdopendir() functions return a pointer to the directory stream.  On error, NULL is re‐
	turned, and errno is set to indicate the error.
---

### `readdir` - read a directory

Synopsis:

```C
#include <dirent.h>

struct dirent *readdir(DIR *dirp);
```

Description:

	The readdir() function returns a pointer to a dirent structure representing the next directory entry in the
	directory  stream pointed to by dirp.  It returns NULL on reaching the end of the directory stream or if an
	error occurred.

	In the glibc implementation, the dirent structure is defined as follows:

		struct dirent {
			ino_t          d_ino;       /* Inode number */
			off_t          d_off;       /* Not an offset; see below */
			unsigned short d_reclen;    /* Length of this record */
			unsigned char  d_type;      /* Type of file; not supported
											by all filesystem types */
			char           d_name[256]; /* Null-terminated filename */
		};

Return:

	On  success,  readdir()  returns  a pointer to a dirent structure.  (This structure may be statically allo‐
	cated; do not attempt to free(3) it.)

	If the end of the directory stream is reached, NULL is returned and errno is not changed.  If an error  oc‐
	curs, NULL is returned and errno is set to indicate the error.  To distinguish end of stream from an error,
	set errno to zero before calling readdir() and then check the value of errno if NULL is returned.
---

### `closedir` - close a directory

Synopsis:

```C
#include <sys/types.h>
#include <dirent.h>

int closedir(DIR *dirp);
```

Description:

	The  closedir() function closes the directory stream associated with dirp.  A successful call to closedir()
	also closes the underlying file descriptor associated with dirp.  The directory stream descriptor  dirp  is
	not available after this call.

Return:

	The  closedir()  function returns 0 on success.  On error, -1 is returned, and errno is set to indicate the
	error.
---
