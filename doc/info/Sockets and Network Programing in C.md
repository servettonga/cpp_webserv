# Sockets and Network Programming in C

In this hyper-connected electronic world, knowing how to send and receive data remotely with sockets is crucial. In this article, we will see how a socket is essentially a digital “plug” that we can attach to a local or remote address in order to establish a connection. We will also explore the architecture and system calls that allow us to create not only a client but also a server in the C programming language.

# What are Sockets?

Everyone has probably heard the saying that in Unix systems, “everything is a file”. Sockets are no exception. Indeed, a socket is simply a file descriptor that enables remote communication. There are several different types of sockets, but in this article, we will concentrate on Internet sockets.

Of course, there are also many types of Internet sockets which all have different ways to transmit data. Among them, the two main types are:

- **Stream sockets** (`SOCK_STREAM`), which use the TCP in order to communicate. This protocol enforces a reliable and connected transmission of data, at the cost of a slightly reduced performance.
- **Datagram sockets** (`SOCK_DGRAM`), which use UDP. As opposed to TCP, UDP allows connection-less data transmission, which is fast, but without any guarantees.

In this article, we will mainly focus on stream sockets, and see how we can use them for remote communication.

# The Importance of Byte Order

Whenever we wish to send and receive data from one computer to another, we must keep in mind that systems can represent their data in two distinct and opposite ways. Take for example the hexadecimal integer `2F0A` (which is 12042 in decimal). Because of its size, this integer must be stored over two bytes: `2F` and `0A`.

It is logical to assume that this integer will always be stored in this order: `2F`, followed by `0A`. This is the most common ordering, known as “big endian” since the big end of the number, the most significant byte, is stored first. But this is not always the case…

In some systems, particularly those with an Intel or Intel-compatible processor, prefer storing the bytes of our integer in the opposite order, with the least significant, or small end, first: `0A` followed by `2F`. We call this ordering “little endian”.

|                                      | **First Byte** | **Second Byte** |
| ------------------------------------ | -------------- | --------------- |
| Big Endian:– hexadecimal– binary     | 2F 00101111    | 0A 00001010     |
| Little Endian :– hexadecimal– binary | 0A 00001010    | 2F 00101111     |

This potentially incompatible difference between host systems can of course cause some issues during data transfer.

The Network Byte Order is always big endian. But the Host Byte Order can be either big endian, or little endian, depending on its architecture.

### Converting to and from Network Byte Order

Thankfully, we can simply assume that the host system is not storing its bytes in the right order compared to the network. All we then need to do is to systematically reorder these bytes when we transfer them between the network and host systems. For this, we can make use of four handy functions from the `<arpa/inet.h>` library:

```cpp
uint32_t htonl(uint32_t hostlong);  //"Host to network long"
uint16_t htons(uint16_t hostshort); //"Host to network short"
uint32_t ntohl(uint32_t netlong);   //"Network to host long"
uint16_t ntohs(uint16_t netshort);  //"Network to host short"
```

As we can see, these functions come in two variants: the ones that convert a `short` (two bytes, or 16 bits), and those that convert a `long` (four bytes or 32 bits). They also work for unsigned integers.

In order to convert a four byte (32 bit) integer from the Host Byte Order to the Network Byte Order, we’ll want to call the `htonl()` function (“htonl” stands for “Host to Network Long”). For the opposite operation, we’d use `ntohl()` (“Network to Host Long”).

With this word of warning in mind, we can now turn to the issue of establishing a connection within our program.

# Preparing a Connection

Whether our program is a server or a client, the first thing we need to do is prepare a small data structure. This structure will contain the information that our socket will need: notably, the IP address and the port to connect to.

### Structures for the Connection IP Address and Port

The basic structures we need to use in order to hold the IP address and port we want to connect to can be found in the `<netinet/in.h>` library. There are two variants of them: one for IPv4 and one for IPv6.

### For an IPv4 Address

For an IPv4 address, we will use the `sockaddr_in` structure, which is defined as follows:

```cpp
// IPv4 only (see sockaddr_in6 for IPv6)
struct sockaddr_in {
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
};
struct in_addr {
    uint32_t       s_addr;
};
```

Let’s take a closer look at what we need to supply to this structure:

- `sin_family`: represents the family of protocols of the IP address, meaning the version of the Internet protocol: either IPv4 or IPv6. Since this structure can only be used for IPv4 addresses, we will always indicate the `AF_INET` constant.
- `sin_port`: the port to which we would like to connect. Warning: we must supply this port number in the *Network Byte Order*, not the host order. For example, to connect to port 3302, we will need to use `htons()` to indicate it here: `htons(3302)`.
- `sin_addr`: a small structure of type `in_addr`, which contains the integer representation of an IPv4 address.

There is only one field to fill out in the `in_addr` structure: `s_addr`. It is a Network Byte Order integer that represents an IPv4 address. We will examine below how to convert an IP address into an integer. However, there are a few constants that we could use here (without forgetting to convert the order of their bytes with `htonl()` !):

- `INADDR_LOOPBACK`: the local machine’s IP address: localhost, or `127.0.0.1`
- `INADDR_ANY`: the IP address `0.0.0.0`
- `INADDR_BROADCAST`: the IP address `255.255.255.255`

### For an IPv6 Address

A similar structure exists to specify an IPv6 address:

```cpp
// IPv6 only (see sockaddr_in for IPv4)
struct sockaddr_in6 {
    sa_family_t     sin6_family;
    in_port_t       sin6_port;
    uint32_t        sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t        sin6_scope_id;
};
struct in6_addr {
    unsigned char   s6_addr[16];
};
```

This structure, `sockaddr_in6`, expects the same information as the previous IPv4 structure. We won’t linger on the two new fields, `sin6_flowinfo` and `sin6_scope_id`, since this is an introduction to sockets.

Just like for IPv4, there are global variables that we can give as the IPv6 address in the `in6_addr` structure: `in6addr_loopback` and `in6addr_any`.

### Converting an IP Address to an Integer

An IPv4 address such as `216.58.192.3` (or an IPv6 address like `2001:db8:0:85a3::ac1f:8001`) is not an integer. It’s a string of characters. In order to convert this string to an integer that we can use in one of the previous structures, we need to call a function from the `<arpa/inet.h>` library: `inet_pton()` (“pton” stands for “presentation to network”).

```cpp
int inet_pton(int af, const char * src, void *dst);
```

Let’s take a closer look at its parameters:

- `af`: an integer that represents the address’ family of Internet protocols. For an IPv4 address, we will want `AF_INET` here; for an IPv6 address, `AF_INET6`.
- `src`: a string containing the IPv4 or IPv6 address to convert.
- `dst`: a pointer to an `in_addr` (IPv4) or `in6_addr` (IPv6) structure in which to store the result of the conversion.

The `inet_pton()` function returns:

- 1 to indicate success,
- 0 if *src* does not contain a valid address for the indicated address family,
- 1 if `af` is not a valid address family, in which `ase` it sets errno to `EAFNOSUPPORT`.

To convert an IPv4 address, we can do something like this:

```cpp
// IPv4 onlystruct sockaddr_in sa;
inet_pton(AF_INET, "216.58.192.3", &(sa.sin_addr));
```

For an IPv6 address:

```cpp
// IPv6 onlystruct sockaddr_in6 sa;
inet_pton(AF_INET6, "2001:db8:0:85a3::ac1f:8001", &(sa.sin6_addr));
```

Of course, the opposite function exists as well, `inet_ntop()` (“ntop” meaning “network to presentation”). It allows us to convert an integer into a legible IP address.

But what if we don’t actually know the IP address we want to connect with? Maybe we only have a domain name such as `http://www.example.com`…

### Automatically Fill In the IP Address with `getaddrinfo()`

If we don’t know the precise IP address we wish to connect to, the `getaddrinfo()` function from the `<netdb.h>` library will be able to help us. Among other things, it allows us to supply a domain name (`http://www.example.com`) instead of an IP address. Calling `getaddrinfo()` will have a slight performance cost since it usually needs to check the DNS to fill out the IP address for us. Its prototype is as follows:

```cpp
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);
```

This function is not as complicated as it may seem. It’s parameters are:

- `node`: a string representing an IP address (IPv4 or IPv6), or a domain name like `www.example.com`. Here, we can also indicate `NULL` if we supply the right flags to the *hints* structure described below (for example, if we want to automatically fill the localhost address).
- `service`: a string representing the port we’d like to connect to, such as “80”, or the name of the service to connect to, like “http”. On a Unix system, the list of available services and their ports can be found in `/etc/services`. This list is also available online on [iana.org](https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml).
- `hints`: a pointer to an `addrinfo` structure where we can supply more information about the connection we want to establish. The hints we provide here act as a kind of search filter for `getaddrinfo()`. We will examine this structure more closely below.
- `res`: a pointer to a linked list of `addrinfo` structures where `getaddrinfo()` can store its result(s).

The `getaddrinfo()` function returns `0` on success, or an error code on failure. The `gai_strerror()` can translate the error returned by `getaddrinfo()` into a readable string.

In addition, the `freeaddrinfo()` function allows us to free the memory of the `addrinfo` once we’re done with it. Let’s examine that structure now.

### The `addrinfo` Structure

Two of `getaddrinfo()`‘s parameters, *hints* and *res*, are pointers towards the same type of structure. So let’s try to understand it:

```cpp
struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    size_t           ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;
    struct addrinfo *ai_next;
};
```

The `addrinfo` structure contains the following elements:

- `ai_flags`: flags containing options that we can combine with a bitwise OR. Here, the most interesting flag that we might want to use is `AI_PASSIVE`, which indicates that the socket we will create will be used to listen for and accept connections in the context of a socket. In this case, we won’t need to indicate an IP address when we call `getaddrinfo()`, since the IP will be that of the machine. A list of all of the available flags can be found on the [manual page for getaddrinfo()](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html).
- `ai_family`: the address family of Internet protocols. To force an IPv4 address, we can indicate `AF_INET`; to force an IPv6 address, `AF_INET6`. One advantage here is that we can make our code IP-agnostic by indicating `AF_UNSPEC`. In this case, `getaddrinfo()` will return IPv4 and IPv6 addresses.
- `ai_socktype`: the type of socket we’d like to create with the address. Two constants are available *here*: `SOCK_STREAM`, which uses TCP, and `SOCK_DGRAM`, which uses UDP. We can also indicate 0 here, to tell `getaddrinfo()` that it can return any type of socket address.
- `ai_protocol`: the protocol of the socket address. In general, there is only one valid protocol for each socket type – TCP for a stream socket, UDP for a datagram socket -, so we can safely put 0 here to tell `getaddrinfo()` that is can return any type of addresses.
- `ai_addrlen`: `getaddrinfo()` will indicate the length of the address here.
- `ai_addr`: a pointer to a `sockaddr_in` or `sockaddr_in6`, which we’ve seen previously, which `getaddrinfo()` will fill out for us.
- `ai_canonnam`**e**: only used if the `AI_CANONNAME` flag is set in *ai_flags*.
- **ai_next**: a pointer to the next element in the linked list.

### `getaddrinfo()` Example

Let’s write a small program that prints the IP addresses for a domain name:

```cpp
// showip.c -- a simple programme the shows a domain name's IP address(es)
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main(int ac, char **av) {
    struct addrinfo hints; // Hints or "filters" for getaddrinfo()
    struct addrinfo *res;  // Result of getaddrinfo()
    struct addrinfo *r;    // Pointer to iterate on results
    int status; // Return value of getaddrinfo()
    char buffer[INET6_ADDRSTRLEN]; // Buffer to convert IP address

    if (ac != 2) {
        fprintf(stderr, "usage: /a.out hostname\n");
        return (1);
    }

    memset(&hints, 0, sizeof hints); // Initialize the structure
    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    // Get the associated IP address(es)
    status = getaddrinfo(av[1], 0, &hints, &res);
    if (status != 0) { // error !
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return (2);
    }

    printf("IP addresses for %s:\n", av[1]);

    r = res;
    while (r != NULL) {
        void *addr; // Pointer to IP address
        if (r->ai_family == AF_INET) { // IPv4
            // we need to cast the address as a sockaddr_in structure to
            // get the IP address, since ai_addr might be either
            // sockaddr_in (IPv4) or sockaddr_in6 (IPv6)
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)r->ai_addr;
            // Convert the integer into a legible IP address string
            inet_ntop(r->ai_family, &(ipv4->sin_addr), buffer, sizeof buffer);
            printf("IPv4: %s\n", buffer);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)r->ai_addr;
            inet_ntop(r->ai_family, &(ipv6->sin6_addr), buffer, sizeof buffer);
            printf("IPv6: %s\n", buffer);
        }
        r = r->ai_next; // Next address in getaddrinfo()'s results
    }
    freeaddrinfo(res); // Free memory
    return (0);
}
```

When we run this program with a domain name as an argument, we receive the associated IP address(es) as a result:

```other
$ gcc showip.c -o showip
$ ./showip example.com
IP addresses for example.com:
IPv4: 103.224.182.219
$ ./showip google.com
IP addresses for google.com:
IPv6: 2a00:1450:4007:813:200e
IPv4: 142.250.178.142
$ ./showip a
getaddrinfo: Name of service not known
```

Now that we know how to get the IP address and store it in the appropriate structure, we can turn our attention to preparing our socket in order to actually establish our connection.

# Preparing Sockets

At last, we can create the file descriptor for our socket. With it, we will be able to read and write in order to respectively receive and sent data. The system call from the `<sys/socket.h>` library, simply named `socket()`, is what we need! This is its prototype:

```cpp
int socket(int domain, int type, int protocol);
```

The parameters it requires are as follows:

- **domain**: the socket’s protocol family, generally `PF_INET` or `PF_INET6`. `PF_INET` exists for historical reasons and is, in practice, identical to `AF_INET`. The same is true for `PF_INET6`.
- **type**: the type of socket, generally `SOCK_STREAM` or `SOCK_DGRAM`.
- **protocol**: the protocol to use with the socket. In general, there is only one valid protocol by socket type, TCP for a stream socket and UDP for a datagram socket, which means we can safely put 0 here.

The `socket()` function returns the file descriptor of the new socket. In case of failure, it returns -1 and indicates the error it encountered in `errno`.

In practice, we probably won’t be filling out the `socket()` function’s parameters manually. Not when we can simply indicate the values returned by `getaddrinfo()`, a little like this:

```cpp
int status;
int socket_fd;
struct addrinfo hints;
struct addrinfo *res;

// fill out hints to prepare getaddrinfo() call

status = getaddrinfo("www.example.com", "http", &hints, &res);
// check if getaddrinfo() failed

socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
// check if socket() failed
```

But this socket file descriptor is not yet connected to anything at all. Naturally, we will want to associate it to a socket address (meaning an IP address and port combination). For this, we have two choices:

- Connect the socket to a remote address with `connect()`. This will allow it to work as a client, able to make requests to a remote server.
- Connect the socket to a local address with `bind()`. In this case, it will work as a server, able to accept connections from remote clients.

Diagram of a connection between a server and a client.

![Image.png](https://res.craft.do/user/full/930b54ea-d199-a247-cdf4-172b3dbd5f59/doc/F0E2F13F-E81C-416D-ADEF-205D1CFC44E3/EB648055-5425-4AEA-82EC-53D94E3F4B32_2/r2a608ggdwLtLlrSwN1m3fbwtTa9CzuYnK1ynOE1Rgcz/Image.png)

Let’s first explore the client side, then we can take a look at the server side.

# Client Side: Connecting to a Server via a Socket

In order to develop a client, we only need one socket connected to a remote server. All we have to do is use the `connect()` system call from the `<sys/socket.h>` library:

```cpp
int connect(int sockfd, const struct sockaddr *serv_addr,
            socklen_t addrlen);
```

Its parameters are quite intuitive:

- `sockfd`: the socket file descriptor we got from our call to `socket()`,
- `serv_addr`: a pointer to the structure containing the connection information. This will either be `sockaddr_in` for an IPv4 address, or `sockaddr_in6` for an IPv6 address.
- `addrlen`: the size in bytes of the previous structure, `serv_addr`.

The function predictably returns 0 in for success and -1 for failure, with `errno` set to indicate the error.

Once more, all of the data needed for the connection can be found in the structure returned by `getaddrinfo()`:

```cpp
int status;
int socket_fd;
struct addrinfo hints;
struct addrinfo *res;

// fill out hints to prepare getaddrinfo() call

status = getaddrinfo("www.example.com", "http", &hints, &res);
// check if getaddrinfo() failed

socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
// check if socket() failed

connect(socket_fd, res->ai_addr, res->ai_addrlen);
```

There we go! Our socket is now ready to send and receive data. But before we get to how to do that, let’s first take a look at establishing a connection from the server side.

# Server Side: Accepting Client Connections via a Socket

If we want to develop a server, the connection will need to be done in three steps. First, we need to bind our socket to a local address and port. Then, we’ll have to listen to the port to detect incoming connection requests. And finally, we need to accept those client connection requests.

### Binding the Socket to the Local Address

The `bind()` function of `<sys/socket.h>` allows us to link our socket to a local address and port. Its prototype is practically identical to its twin function `connect()`, which we examined for the client side:

```cpp
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

Just like `connect()`, the parameters of `bind()` are:

- `sockfd`: the socket file descriptor we got with our call to `socket()`.
- `addr`: a pointer to the structure containing the connection information. This is either a `sockaddr_in` for an IPv4 address, or a `sockaddr_in6` for an IPv6 address.
- `addrlen`: the size in bytes of the previous structure, `addr`.

As expected, the function returns 0 for success and -1 to indicate an error, with the error code in `errno`.

### Listening via a Socket to Detect Connection Requests

Next, we need to mark the socket as “passive”, meaning it will be used to accept incoming connection requests on the address and port it is bound to. For this, we will use the `listen()` function, which is also in `<sys/socket.h>`.

```cpp
int listen(int sockfd, int backlog);
```

The `listen()` function takes two parameters:

- `sockfd`: the socket file descriptor that we got with the call to `socket()`.
- `backlong`: an integer representing the number of connection requests allowed in the queue. Incoming connections will be placed in this queue until we accept them. Most systems automatically cap the number of pending connection requests to 20, but we can manually specify this maximum as we wish.

If the call to `listen()` succeeds, the function returns 0. If it fails, it returns -1 and sets `errno` accordingly.

### Accepting a Client Connection

Finally, we must accept the connection requests from a remote client. When a client `connect()`s on the port of our machine that our socket is `listen()`ing to, its request is put in the pending queue. When we `accept()` the request, that function will return a new file descriptor bound to the client’s address, through which we will be able to communicate with that client. So we will end up with two file descriptors: our initial socket that will continue listening to our port, and a new file descriptor for the client, which we can use to send and receive data.

The prototype of the `accept()` function of `<sys/socket.h>` is as follows:

```cpp
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

Let’s take a closer look at its parameters:

- `sockfd`: the listening socket’s file descriptor, which we got with the call to `socket()`.
- `addr`: a pointer to a `sockaddr` structure where `accept()` can fill out the information related to the client socket. If we don’t want to save the client’s address or port number, we can just put `NULL` here.
- `addrlen`: a pointer to an integer which contains the size in bytes of the previous structure. `accept()` will adjust this value if it is too large for the final size of the structure, but it will truncate the address if this value is smaller than the final size of the address.

The `accept()` function returns the file descriptor of the new socket, or -1 in case it encounters an error, which it indicates in `errno`.

### Server Socket Example

Let’s create a micro-server which can accept a connection request with calls to the `bind()`, `listen()`, and `accept()` functions:

With `getaddrinfo()`

```cpp
// server.c - a micro-server that accepts a connection before quitting
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT "4242" // our server's port
#define BACKLOG 10  // max number of connection requests in queue

int main(void)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int socket_fd;
    int client_fd;
    int status;
    // sockaddr_storage is a structure that is not associated to
    // a particular family. This allows us to receive either
    // an IPv4 or an IPv6 address
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    // Prepare the address and port for the server socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // Automatically fills IP address

    status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\\n", gai_strerror(status));
        return (1);
    }

    // create socket, bind it and listen with it
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    status = bind(socket_fd, res->ai_addr, res->ai_addrlen);
    if (status != 0) {
        fprintf(stderr, "bind: %s\\n", strerror(errno));
        return (2);
    }
    listen(socket_fd, BACKLOG);

    // Accept incoming connection
    addr_size = sizeof client_addr;
    client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd == -1) {
        fprintf(stderr, "accept: %s\\n", strerror(errno));
        return (3);
    }
    printf("New connection! Socket fd: %d, client fd: %d\\n", socket_fd, client_fd);

    // We are ready to communicate with the client via the client_fd!

    return (0);
}
```

Without `getaddrinfo()`

```cpp
// server.c - a micro-server that accepts a connection before quitting
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT "4242" // our server's port
#define BACKLOG 10  // max number of connection requests in queue

int main(void)
{
    struct sockaddr_in sa;
    int socket_fd;
    int client_fd;
    int status;
    // sockaddr_storage is a structure that is not associated to
    // a particular family. This allows us to receive either
    // an IPv4 or an IPv6 address
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; // IPv4 only; use AF_INET6 for IPv6
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // create socket, bind it and listen with it
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0) {
        fprintf(stderr, "bind: %s\\n", strerror(errno));
        return (2);
    }
    listen(socket_fd, BACKLOG);

    // Accept incoming connection
    addr_size = sizeof client_addr;
    client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd == -1) {
        fprintf(stderr, "accept: %s\\n", strerror(errno));
        return (3);
    }
    printf("New connection! Socket fd: %d, client fd: %d\\n", socket_fd, client_fd);

    // We are ready to communicate with the client via the client_fd!

    return (0);
}
```

When we compile and run this program, we’ll notice it looks to be idling. That’s because the `accept()` function is blocking the execution, waiting for a connection request. This will become an important factor when dealing with multiple client connections, and we will examine ways to handle this at the end of this article.

To simulate a connection request, we can open a new terminal and run the `nc` command (`netcat`), specifying the address of our local machine, localhost (or `127.0.0.1`), and the port on which our little server is running – in this example, it’s `4242`.

```shell
$ gcc server.c -o server && ./server
New connection! Socket fd: 3, client fd: 4
$ nc localhost 8080
```

There we go! Our server has detected and accepted the new connection. and displays the file descriptors of both our server’s listening socket and of the new client socket.

# Sending and Receiving Data Through Sockets

It’s not worth establishing a connection from a client to a server or vice-versa if we don’t know how to send and receive data. Clever readers might notice that since sockets are just file descriptors, we could probably just use the `read()` and `write()` system calls. And they’d be totally correct! But other functions exist, ones that give us more control over the way in which our data is sent and received…

### Sending Data via a Socket

The `send()` function from the `<sys/socket.h>` library allows us to send data through a stream socket, which uses a TCP connection.

```cpp
ssize_t send(int socket, const void *buf, size_t len, int flags);
```

Its parameters are as follows:

- `socket`: the file descriptor of the socket through which we’d like to send data. In a client program, this will be the `fd` we got from our call to `socket()`; on the server side, this will instead be the client `fd` we got from our call to `accept()`.
- `buf`: the buffer or string containing the message to send.
- `len`: an integer representing the size in bytes of the message to send.
- `flags`: an integer containing flags about the way the message should be transmitted. A list of valid flags is available on the `[send()` manual page]([https://man7.org/linux/man-pages/man2/send.2.html](https://man7.org/linux/man-pages/man2/send.2.html)). Usually, we only can get away with setting 0 here.

The `send()` function returns the number of bytes that were successfully sent. *Beware, `send()` might not be able to send the entire message in one go!* This means that we’ll have to be careful to compare the value returned here with the length of the message we wish to send, in order to try sending the rest again if need be. As usual, this function can also return -1 if there is an error, and we can check `errno` for details.

For datagram sockets, the ones that use the connection-less protocol UDP, there is a similar function: `sendto()`. In addition to the above parameters, it also takes the destination address in the form of a `sockaddr` type structure.

### Receiving Data via a Socket

Just like the opposite function `send()`, `recv()` can be found in `<sys/socket.h>`. This function allows us to receive data through a socket. Its prototype is as follows:

```cpp
ssize_t recv(int socket, void *buf, ssize_t len, int flags);
```

The parameters of `recv()` are:

- `socket`: the file descriptor of the socket through which we’d like to send data. In a client program, this will be the `fd` we got from our call to `socket()`; on the server side, this will instead be the client `fd` we got from our call to `accept()`.
- `buf`: a pointer to a buffer, a memory area, where we can store the read data.
- `len`: the size in bytes of the previous buffer.
- `flags`: the flags relating to message reception. Usually, we’ll only need to supply 0, here. See the `[recv()` manual page]([https://man7.org/linux/man-pages/man2/recv.2.html](https://man7.org/linux/man-pages/man2/recv.2.html)) for a list of available flags.

Just like `send()`, `recv()` returns the number of bytes it managed to store in the buffer. However, if `recv()` returns 0, it can only mean one thing: the remote computer has closed the connection. Naturally, the `recv()` function can also return -1 if it encounters an error, in which case it sets the error code in `errno`.

There is also another similar function, `recvfrom()`, for datagram type sockets which use the connection-less protocol UDP. On top of the parameters we supplied for `recv()`, this function also needs to know the source address from which the message is expected to be sent, in a `sockaddr` structure.

# Closing a Socket Connection

Once we are done sending and receiving data, we will be able to close our socket. Just as any other file descriptor, a socket can be closed with a simple call to `close()` from `<unistd.h>`. This destroys the file descriptor and prevents any further communication with the socket: the remote side will raise an exception if it attempts to communicate with it by sending or receiving data.

But there is another function that is worth mentioning: `shutdown()` from `<sys/socket.h>`. This function gives us more control over how we close our socket. Its prototype is:

```cpp
int shutdown(int sockfd, int how);
```

Its parameters are simple enough:

- `sockfd`: the socket file descriptor we want to close.
- `how`: an integer containing flags indicating how to close the socket. Valid flags here are:
   - `SHUT_RD` to close the read end of the socket and prevent data reception.
   - `SHUT_WR` to close the write end of the socket and prevent data transmission.
   - `SHUT_RDWR` to close both ends of the socket and prevent data reception and transmission.

However, we must keep in mind that `shutdown()` does not destroy the socket file descriptor or free the memory associated with it. It only modifies its read and write permissions. A call to `close()` is therefore still necessary.

Like any self-respecting system call, `shutdown()` returns 0 on success and -1 on failure, with `errno` containing the error code.

# Socket Communication Example Between a Server and a Client

Using everything we have learned about sockets up until now, let’s create two small programs. The first will be a server that will listen on our local machine’s port 4242. It will accept a client connection and will wait to receive a message to which it will then respond. The second program will be a client, and we will pass as an argument the message to send to the server. Then, the client will wait for the server reply before closing the connection.

`server.c`

```cpp
// server.c - a micro-server that accepts a client connection, waits for a message, and replies
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080  // our server's port
#define BACKLOG 10  // max number of connection requests in queue

int main(void)
{
    printf("---- SERVER ----\n\n");
    struct sockaddr_in sa;
    int socket_fd;
    int client_fd;
    int status;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    char buffer[BUFSIZ];
    int bytes_read;

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // Create socket, bind it and listen with it
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "socket fd error: %s\n", strerror(errno));
        return (1);
    }
    printf("Created server socket fd: %d\n", socket_fd);

    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        return (2);
    }
    printf("Bound socket to localhost port %d\n", PORT);

    printf("Listening on port %d\n", PORT);
    status = listen(socket_fd, BACKLOG);
    if (status != 0) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return (3);
    }

    // Accept incoming connection
    addr_size = sizeof client_addr;
    client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);
    if (client_fd == -1) {
        fprintf(stderr, "client fd error: %s\n", strerror(errno));
        return (4);
    }
    printf("Accepted new connection on client socket fd: %d\n", client_fd);

    // Receive message from client socket
    bytes_read = 1;
    while (bytes_read >= 0) {
        printf("Reading client socket %d\n", client_fd);
        bytes_read = recv(client_fd, buffer, BUFSIZ, 0);
        if (bytes_read == 0) {
            printf("Client socket %d: closed connection.\n", client_fd);
            break ;
        }
        else if (bytes_read == -1) {
            fprintf(stderr, "recv error: %s\n", strerror(errno));
            break ;
        }
        else {
            // We got a message, print it
            // and send a message back to client
            char *msg = "Got your message.";
            int msg_len = strlen(msg);
            int bytes_sent;

            buffer[bytes_read] = '\0';
            printf("Message received from client socket %d: \"%s\"\n", client_fd, buffer);

            bytes_sent = send(client_fd, msg, msg_len, 0);
            if (bytes_sent == -1) {
                fprintf(stderr, "send error: %s\n", strerror(errno));
            }
            else if (bytes_sent == msg_len) {
                printf("Sent full message to client socket %d: \"%s\"\n", client_fd, msg);
            }
            else {
                printf("Sent partial message to client socket %d: %d bytes sent.\n", client_fd, bytes_sent);
            }
        }
    }

    printf("Closing client socket\n");
    close(client_fd);
    printf("Closing server socket\n");
    close(socket_fd);

    return (0);
}
```

`client.c`

```cpp
// client.c - a micro-client that sends a message to a server and awaits a reply
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080  // server port to connect to

int main(int ac, char **av)
{
    printf("---- CLIENT ----\n\n");
    struct sockaddr_in sa;
    int socket_fd;
    int status;
    char buffer[BUFSIZ];
    int bytes_read;
    char *msg;
    int msg_len;
    int bytes_sent;

    if (ac != 2) {
        printf("Usage: ./client \"Message to send\"");
        return (1);
    }

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // Create socket, connect it to remote server
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "socket fd error: %s\n", strerror(errno));
        return (2);
    }
    printf("Created socket fd: %d\n", socket_fd);

    status = connect(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0) {
        fprintf(stderr, "connect error: %s\n", strerror(errno));
        return (3);
    }
    printf("Connected socket to localhost port %d\n", PORT);

    // Send a message to server
    msg = av[1];
    msg_len = strlen(msg);
    bytes_sent = send(socket_fd, msg, msg_len, 0);
    if (bytes_sent == -1) {
        fprintf(stderr, "send error: %s\n", strerror(errno));
    }
    else if (bytes_sent == msg_len) {
        printf("Sent full message: \"%s\"\n", msg);
    }
    else {
        printf("Sent partial message: %d bytes sent.\n", bytes_sent);
    }

    // Wait for message from server via the socket
    bytes_read = 1;
    while (bytes_read >= 0) {
        bytes_read = recv(socket_fd, buffer, BUFSIZ, 0);
        if (bytes_read == 0) {
            printf("Server closed connection.\n");
            break ;
        }
        else if (bytes_read == -1) {
            fprintf(stderr, "recv error: %s\n", strerror(errno));
            break ;
        }
        else {
            // We got a message, print it
            buffer[bytes_read] = '\0';
            printf("Message received: \"%s\"\n", buffer);
            break ;
        }
    }

    printf("Closing socket\n");
    close(socket_fd);
    return (0);
}
```

The server receives the message we ask the client to send, and manages to reply with its confirmation! The client then closes its socket, which the server is able to detect since `recv()` returns `0`.

# I/O Multiplexing: Handling Several Sockets Without Blocking

We might have noticed during our tests that a large number of the functions we use with sockets, like `accept()` and `recv()` are blocking. This means that they suspend our process until their own executions are finished. If we run our micro-server all by itself, without ever connecting a client, it will stay indefinitely blocked on `accept()` since that system call will wait for a connection request that will never come.

This is not inherently a bad thing, but it can cause issues for a server that attempts to handle several clients at the same time. If we are waiting to receive data from a client, it should not stop us from accepting a new connection or another message from another client. This is another aspect of concurrency, where one program must deal with several things simultaneously.

Thankfully, there are “multiplexing” methods that can make our sockets non-blocking, and that can detect when they are ready to be used.

### Making Sockets Non-Blocking with `fcntl()`

When we invoke the `socket()` system call to get a file descriptor for our socket, the operating system’s kernel automatically creates it as “blocking”. If we so desire, we can transform it into a non-blocking socket with the file descriptor manipulation function `fcntl()` from `<unistd.h>` and `<fcntl.h>`, like this:

```cpp
...
socket_fd = socket(PF_INET, SOCK_STREAM, 0);
fcntl(socket_fd, F_SETFL, O_NONBLOCK);
...
```

When we make the socket non-blocking with `O_NONBLOCK`, it prevents system calls like `recv()` from suspending out program during their executions. If there is nothing to read from the socket, `recv()` immediately returns with -1 and sets `errno` to `EAGAIN` or `EWOULDBLOCK`. With this information, we can loop over our sockets one by one to check if they have anything for us to read, and if not, we move onto the next. The same is possible for any blocking function such as `accept()`, for example.

However, looping over our client sockets in this way is a very intensive operation for out poor CPU, especially if there are hundreds! There are much better ways to handle this blocking issue, which we will discover now.

### Monitoring Sockets with `select()`

What would be convenient is a way to monitor all of our socket file descriptors and be notified when one of them is ready for an operation. After all, if we know the socket is ready, we can read or write to it without any risk of blocking.

This is exactly what the `select()` does. In order to use it, we’ll want to import `<sys/select.h>`, `<sys/time.h>`, `<sys/types.h>`, and `<unistd.h>`. Its prototype is:

```cpp
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
```

Its parameters can seem a bit complicated, so let’s take a closer look at them:

- `nfds`: an integer indicating the value of the highest file descriptor to monitor, plus one.
- `readfds`: a set of file descriptors to monitor for reading, to make sure a call to `read(`) or `recv()` will not block. Can be `NULL`.
- `writefds`: a set of file descriptors to monitor for writing, to ensure a call to `write()` or `send()` will not block. Can be `NULL`.
- `exceptfds`: a set of file descriptors to monitor for exceptions. Can be `NULL`.
- **timeout**: the delay after which we force `select()` to finish its execution if no file descriptors in any set change states.

If it succeeds, `select()` modifies each set to indicate which file descriptors are ready for an operation. It also returns the total number of file descriptors which are ready among the three sets. If none of the descriptors are ready before the timeout is reached, `select()` can return 0.

If it encounters an error, `select()` returns `-1` and sets `errno`. In this case, it does not modify the file descriptor sets.

### Manipulating File Descriptor Sets for `select()`

In order to manipulate the file descriptor sets that we want to monitor with `select()`, we will want to use the following macros:

```cpp
void FD_CLR(int fd, fd_set *set);   // Removes an fd from the set
int  FD_ISSET(int fd, fd_set *set); // Checks if an fd is part of the set
void FD_SET(int fd, fd_set *set);   // Adds an fd to the set
void FD_ZERO(fd_set *set);          // Sets the set to 0
```

### `select()`’s Timeout

The *timeout* parameter represents the maximum time limit spent in the `select()` function. Past this deadline, if none of the file descriptors of the sets it monitors become ready, `select()` will return. We will then be able to do other things, such as print a message to confirm we are still waiting.

The structure to use for the temporal value, `timeval`, can be found in `<sys/time.h>`:

```cpp
struct timeval {
    long    tv_sec;    // seconds
    long    tv_usec;   // microseconds
};
```

If this time value is set to 0, `select()` will return immediately; if we set it to `NULL`, `select()` will be able to block indefinitely if none of the file descriptors change states.

On some Linux systems, `select()` modifies the *timeval* value upon returning, to reflect the time that is left. This is far from universal, so for portability’s sake, we should not lean on this aspect.

### Socket Monitoring Example with select()

Let’s try to create a small server that monitors each connected socket file descriptor with `select()` for reading. When one of them is ready to read, we will check if it is the server’s listening socket, in which case we will need to accept a new client connection. Indeed, it’s very useful to know that the `listen()`ing socket will be marked as ready to read if there is a connection request to accept! If the socket is a client socket, though, we will read the received message and relay it to all of the other connected sockets.

```cpp
// server.c - a small server that monitors sockets with select() to accept connection requests and relay client messages
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080  // our server's port

int create_server_socket(void);
void accept_new_connection(int listener_socket, fd_set *all_sockets, int *fd_max);
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket);

int main(void)
{
    printf("---- SERVER ----\n\n");

    int server_socket;
    int status;

    // To monitor socket fds:
    fd_set all_sockets; // Set for all sockets connected to server
    fd_set read_fds;    // Temporary set for select()
    int fd_max;         // Highest socket fd
    struct timeval timer;

    // Create server socket
    server_socket = create_server_socket();
    if (server_socket == -1) {
        return (1);
    }

    // Listen to port via socket
    printf("[Server] Listening on port %d\n", PORT);
    status = listen(server_socket, 10);
    if (status != 0) {
        fprintf(stderr, "[Server] Listen error: %s\n", strerror(errno));
        return (3);
    }

    // Prepare socket sets for select()
    FD_ZERO(&all_sockets);
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &all_sockets); // Add listener socket to set
    fd_max = server_socket; // Highest fd is necessarily our socket
    printf("[Server] Set up select fd sets\n");

    while (1) { // Main loop
        // Copy all socket set since select() will modify monitored set
        read_fds = all_sockets;
        // 2 second timeout for select()
        timer.tv_sec = 2;
        timer.tv_usec = 0;

        // Monitor sockets ready for reading
        status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
        if (status == -1) {
            fprintf(stderr, "[Server] Select error: %s\n", strerror(errno));
            exit(1);
        }
        else if (status == 0) {
            // No socket fd is ready to read
            printf("[Server] Waiting...\n");
            continue;
        }

        // Loop over our sockets
        for (int i = 0; i <= fd_max; i++) {
            if (FD_ISSET(i, &read_fds) != 1) {
                // Fd i is not a socket to monitor
                // stop here and continue the loop
                continue ;
            }
            printf("[%d] Ready for I/O operation\n", i);
            // Socket is ready to read!
            if (i == server_socket) {
                // Socket is our server's listener socket
                accept_new_connection(server_socket, &all_sockets, &fd_max);
            }
            else {
                // Socket is a client socket, let's read it
                read_data_from_socket(i, &all_sockets, fd_max, server_socket);
            }
        }
    }
    return (0);
}

// Returns the server socket bound to the address and port we want to listen to
int create_server_socket(void) {
    struct sockaddr_in sa;
    int socket_fd;
    int status;

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // Create the socket
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "[Server] Socket error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Created server socket fd: %d\n", socket_fd);

    // Bind socket to address and port
    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0) {
        fprintf(stderr, "[Server] Bind error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Bound socket to localhost port %d\n", PORT);
    return (socket_fd);
}

// Accepts a new connection and adds the new socket to the set of all sockets
void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max)
{
    int client_fd;
    char msg_to_send[BUFSIZ];
    int status;

    client_fd = accept(server_socket, NULL, NULL);
    if (client_fd == -1) {
        fprintf(stderr, "[Server] Accept error: %s\\n", strerror(errno));
        return ;
    }
    FD_SET(client_fd, all_sockets); // Add the new client socket to the set
    if (client_fd > *fd_max) {
        *fd_max = client_fd; // Update the highest socket
    }
    printf("[Server] Accepted new connection on client socket %d.\n", client_fd);
    memset(&msg_to_send, '\0', sizeof msg_to_send);
    sprintf(msg_to_send, "Welcome. You are client fd [%d]\n", client_fd);
    status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
    if (status == -1) {
        fprintf(stderr, "[Server] Send error to client %d: %s\n", client_fd, strerror(errno));
    }
}

// Read the message from a socket and relay it to all other sockets
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket)
{
    char buffer[BUFSIZ];
    char msg_to_send[BUFSIZ];
    int bytes_read;
    int status;

    memset(&buffer, '\0', sizeof buffer);
    bytes_read = recv(socket, buffer, BUFSIZ, 0);
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            printf("[%d] Client socket closed connection.\n", socket);
        }
        else {
            fprintf(stderr, "[Server] Recv error: %s\n", strerror(errno));
        }
        close(socket); // Close the socket
        FD_CLR(socket, all_sockets); // Remove socket from the set
    }
    else {
        // Relay the received message to all connected sockets
        // but not to the server socket or the sender's socket
        printf("[%d] Got message: %s", socket, buffer);
        memset(&msg_to_send, '\0', sizeof msg_to_send);
        sprintf(msg_to_send, "[%d] says: %s", socket, buffer);
        for (int j = 0; j <= fd_max; j++) {
            if (FD_ISSET(j, all_sockets) && j != server_socket && j != socket) {
                status = send(j, msg_to_send, strlen(msg_to_send), 0);
                if (status == -1) {
                    fprintf(stderr, "[Server] Send error to client fd %d: %s\n", j, strerror(errno));
                }
            }
        }
    }
}
```

We’ll note here that we have two sets of file descriptors: `all_sockets`, which contains all of the connected socket file descriptors, and `read_fds`, the set that will be monitored for reading. The `all_sockets` set is vital in order not to lose file descriptors along the way, since `select()` is known to modify the `read_fds` set in order to tell us which descriptors are ready. So we want to add the file descriptors of each new connection to the `all_sockets` set, and it’s also this set from which we need to remove sockets that have been closed. At the beginning of our main loop, we can copy the contents of `all_sockets` into `read_fds` in order to ensure that `select()` has an up-to-date set of sockets to monitor.

### Testing Socket Monitoring with select()

Let’s test this code by running our little server. In other terminal windows, we can use `nc` as a client to connect to the address and port of our server (in this case, `localhost` port `4242`). We will then be able to send message which should appear for all of the other connected clients:

![server-example-select-socket-1024x846.png](https://res.craft.do/user/full/930b54ea-d199-a247-cdf4-172b3dbd5f59/doc/F0E2F13F-E81C-416D-ADEF-205D1CFC44E3/3C89E7A1-6CD5-4C8D-877A-AF27B64024A2_2/YR1JkdUJHujYDukjMDdZFiqgyg1xaj6ABbYQnmKoHZQz/server-example-select-socket-1024x846.png)

As we can see, our messages are correctly relayed to the other connected clients, without ever blocking. Sometimes, when `select()` finds no file descriptors that are ready to read, the server prints `"Waiting..."`, which shows that `select()` is indeed working as expected.

We have the beginnings of a chat server, here! Ideally, we should also be monitoring our sockets for writing before sending them data, but now that we understand how `select()` works, it won’t be too difficult to implement. For this, we would need to add a second loop after our `select()` call, to check the set of file descriptors we’re monitoring for writing. However, this separation between the different sets of events we want to monitor about our sockets is not ideal…

### Polling Sockets with poll()

The monitoring system we can build with `select()` can be a little inefficient though. Having to loop over three separate sets of file descriptors to know which are ready for reading or writing or have encountered an error is not terribly practical. This is why an alternative to `select()` was developed, one that uses a single structure to monitor any type of change within its file descriptors: `poll()`.

This is the prototype of the `poll()` function from `<poll.h>`:

```cpp
int poll(struct pollfd fds[], nfds_t nfds, int timeout);
```

Its parameters are:

- `fds`: an array of `pollfd` structures, which contain the file descriptors to poll and the events for which we’d like to be notified. We will take a closer look at this structure below.
- `nfds`: an integer representing the number of elements in the previous array.
- **timeout**: an integer representing a temporal value in milliseconds, during which `poll()` will be able to block the execution of our program in order to poll the sockets. Once this deadline has been reached, `poll()` will terminate its execution. Setting this *timeout* to -1 allows `poll()` to block indefinitely as long as no file descriptor has changed states. On the other hand, if we put 0 here, `poll()` will immediately return, even if none of the polled file descriptors are ready.

The `poll()` function returns the number of file descriptors for which it has detected a desired event. If `poll()` returns 0, it means that our *timeout* has expired without any file descriptors being ready. Of course, `poll()` returns -1 on failure and indicates the encountered error in `errno`.

### The `pollfd` Structure

Let’s take a closer look at the `pollfd` structure:

```cpp
struct pollfd {
    int   fd;         // File descriptor
    short events;     // Requested events
    short revents;    // Detected events
};
```

So we will have an array composed of several of these structures where we will indicate the file descriptor of each of our sockets as well as the events we want to request for each of them.

There are several events we can supply in the `events` field. The two most immediately useful ones are the following, but there are others listed on the [manual page for `poll()`](https://man7.org/linux/man-pages/man2/poll.2.html):

- `POLLIN` to be notified if there is data ready to be read with `read()` or `recv()`.
- `POLLOUT` to be notified when a descriptor is ready for writing with `write()` or `send()`.

Of course, we can request any combination of events with a [bitwise OR](https://www.notion.so/Binary-010-The-Uses-of-Bit-Shifting-and-Bitwise-Operations-3a0036089af64a71861525fbcf7aeb62?pvs=21).

Once `poll()` is done with its execution, we can take a look at the `revents` field associated to each element in our array, in order to check if `POLLIN` or `POLLOUT` are set.

### Polling Example with `poll()`

Let’s attempt to replicate our previous server example with `poll()` instead of `select()`. The server will work in the same way: we will add our sockets to the `poll()` array in order to detect when one of them is ready to read. If it’s the server’s listening socket, we’ll accept the incoming connection request. If it’s a client socket, we will read its message and relay it to all other connected sockets.

```cpp
// server-poll.c - a small server that monitors sockets with poll() to accept connection requests and relay client messages
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080  // our server's port

int create_server_socket(void);
void accept_new_connection(int server_socket, struct pollfd **poll_fds, int *poll_count, int *poll_size);
void read_data_from_socket(int i, struct pollfd **poll_fds, int *poll_count, int server_socket);
void add_to_poll_fds(struct pollfd *poll_fds[], int new_fd, int *poll_count, int *poll_size);
void del_from_poll_fds(struct pollfd **poll_fds, int i, int *poll_count);

int main(void)
{
    printf("---- SERVER ----\n");

    int server_socket;
    int status;

    // To monitor client sockets:
    struct pollfd *poll_fds; // Array of socket file descriptors
    int poll_size; // Size of descriptor array
    int poll_count; // Current number of descriptors in array

    // Create server listening socket
    server_socket = create_server_socket();
    if (server_socket == -1) {
        return (1);
    }

    // Listen to port via socket
    printf("[Server] Listening on port %d\n", PORT);
    status = listen(server_socket, 10);
    if (status != 0) {
        fprintf(stderr, "[Server] Listen error: %s\n", strerror(errno));
        return (3);
    }

    // Prepare the array of file descriptors for poll()
    // We'll start with enough room for 5 fds in the array,
    // we'll reallocate if necessary
    poll_size = 5;
    poll_fds = calloc(poll_size + 1, sizeof *poll_fds);
    if (!poll_fds) {
        return (4);
    }
    // Add the listening server socket to array
    // with notification when the socket can be read
    poll_fds[0].fd = server_socket;
    poll_fds[0].events = POLLIN;
    poll_count = 1;

    printf("[Server] Set up poll fd array\n");

    while (1) { // Main loop
        // Poll sockets to see if they are ready (2 second timeout)
        status = poll(poll_fds, poll_count, 2000);
        if (status == -1) {
            fprintf(stderr, "[Server] Poll error: %s\n", strerror(errno));
            exit(1);
        }
        else if (status == 0) {
            // None of the sockets are ready
            printf("[Server] Waiting...\n");
            continue;
        }

        // Loop on our array of sockets
        for (int i = 0; i < poll_count; i++) {
            if ((poll_fds[i].revents & POLLIN) != 1) {
                // The socket is not ready for reading
                // stop here and continue the loop
                continue ;
            }
            printf("[%d] Ready for I/O operation\n", poll_fds[i].fd);
            // The socket is ready for reading!
            if (poll_fds[i].fd == server_socket) {
                // Socket is our listening server socket
                accept_new_connection(server_socket, &poll_fds, &poll_count, &poll_size);
            }
            else {
                // Socket is a client socket, read it
                read_data_from_socket(i, &poll_fds, &poll_count, server_socket);
            }
        }
    }
    return (0);
}

int create_server_socket(void) {
    struct sockaddr_in sa;
    int socket_fd;
    int status;

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET; // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // Create listening socket
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        fprintf(stderr, "[Server] Socket error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Created server socket fd: %d\n", socket_fd);

    // Bind socket to address and port
    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0) {
        fprintf(stderr, "[Server] Bind error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Bound socket to localhost port %d\n", PORT);

    return (socket_fd);
}

void accept_new_connection(int server_socket, struct pollfd **poll_fds, int *poll_count, int *poll_size)
{
    int client_fd;
    char msg_to_send[BUFSIZ];
    int status;

    client_fd = accept(server_socket, NULL, NULL);
    if (client_fd == -1) {
        fprintf(stderr, "[Server] Accept error: %s\n", strerror(errno));
        return ;
    }
    add_to_poll_fds(poll_fds, client_fd, poll_count, poll_size);

    printf("[Server] Accepted new connection on client socket %d.\n", client_fd);

    memset(&msg_to_send, '\0', sizeof msg_to_send);
    sprintf(msg_to_send, "Welcome. You are client fd [%d]\n", client_fd);
    status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
    if (status == -1) {
        fprintf(stderr, "[Server] Send error to client %d: %s\n", client_fd, strerror(errno));
    }
}

void read_data_from_socket(int i, struct pollfd **poll_fds, int *poll_count, int server_socket)
{
    char buffer[BUFSIZ];
    char msg_to_send[BUFSIZ];
    int bytes_read;
    int status;
    int dest_fd;
    int sender_fd;

    sender_fd = (*poll_fds)[i].fd;
    memset(&buffer, '\0', sizeof buffer);
    bytes_read = recv(sender_fd, buffer, BUFSIZ, 0);
    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            printf("[%d] Client socket closed connection.\n", sender_fd);
        }
        else {
            fprintf(stderr, "[Server] Recv error: %s\n", strerror(errno));
        }
        close(sender_fd); // Close socket
        del_from_poll_fds(poll_fds, i, poll_count);
    }
    else {
        // Relays the received message to all connected sockets
        // but not to the server socket or the sender socket
        printf("[%d] Got message: %s", sender_fd, buffer);

        memset(&msg_to_send, '\0', sizeof msg_to_send);
        sprintf(msg_to_send, "[%d] says: %s", sender_fd, buffer);
        for (int j = 0; j < *poll_count; j++) {
            dest_fd = (*poll_fds)[j].fd;
            if (dest_fd != server_socket && dest_fd != sender_fd) {
                status = send(dest_fd, msg_to_send, strlen(msg_to_send), 0);
                if (status == -1) {
                    fprintf(stderr, "[Server] Send error to client fd %d: %s\n", dest_fd, strerror(errno));
                }
            }
        }
    }
}

// Add a new file descriptor to the pollfd array
void add_to_poll_fds(struct pollfd *poll_fds[], int new_fd, int *poll_count, int *poll_size) {
    // If there is not enough room, reallocate the poll_fds array
    if (*poll_count == *poll_size) {
        *poll_size *= 2; // Double its size
        *poll_fds = realloc(*poll_fds, sizeof(**poll_fds) * (*poll_size));
    }
    (*poll_fds)[*poll_count].fd = new_fd;
    (*poll_fds)[*poll_count].events = POLLIN;
    (*poll_count)++;
}

// Remove an fd from the poll_fds array
void del_from_poll_fds(struct pollfd **poll_fds, int i, int *poll_count) {
    // Copy the fd from the end of the array to this index
    (*poll_fds)[i] = (*poll_fds)[*poll_count - 1];
    (*poll_count)--;
}
```

We had to add a couple of small utility functions to handle adding and removing socket file descriptors to and from our `pollfd` array, but apart from that, not much had to change compared to `select()`. Also, the result is identical to our `select()` example, but the execution of our program is now more efficient, since we only have one structure to check to know if a socket is ready to read or write.

![server-example-poll-socket-1-1024x845.png](https://res.craft.do/user/full/930b54ea-d199-a247-cdf4-172b3dbd5f59/doc/F0E2F13F-E81C-416D-ADEF-205D1CFC44E3/5BC72788-F74C-43C2-8ED1-588A57C4CC7E_2/ck9QQCwxT3TcBxLI0HG5xJDcI4svDsVcKVyBLjxFk5Mz/server-example-poll-socket-1-1024x845.png)

However, let’s not forget that for a real server, we would also need to monitor sockets before sending them data! For that, all we’d need to do is add a condition inside of the loop we already have over the `poll_fds` array, which is much more convenient than the multiple different loops that `select()` imposes on us.

### Other Methods

The socket monitoring systems that `select()` and `poll()` provide can both turn out to be very slow the more connections there are, though. To handle a very large number of connections, it’s often preferable to use an event handling library like [libevent](https://libevent.org/).

A little tip to share, a nagging question to ask, or a strange discovery to discuss about sockets, socket file descriptor monitoring, or network programming in general? I’d love to read and respond to it all in the comments. Happy coding !

# Sources and Further Reading

- Linux Programmer’s Manual:
   - *accept (2)* [[man]](https://man7.org/linux/man-pages/man2/accept.2.html)
   - *bind (2)* [[man]](https://man7.org/linux/man-pages/man2/bind.2.html)
   - *byteorder (3)* [[man]](https://man7.org/linux/man-pages/man3/htons.3.html)
   - *connect (2)* [[man]](https://man7.org/linux/man-pages/man2/connect.2.html)
   - *fcntl (2)* [[man]](https://man7.org/linux/man-pages/man2/fcntl.2.html)
   - *getaddrinfo (3)* [[man]](https://man7.org/linux/man-pages/man3/getaddrinfo.3.html)
   - *inet_pton (3)* [[man]](https://man7.org/linux/man-pages/man3/inet_pton.3.html)
   - *IP (7)* [[man]](https://man7.org/linux/man-pages/man7/ip.7.html)
   - *IPv6 (7)* [[man]](https://man7.org/linux/man-pages/man7/ipv6.7.html)
   - *listen (2)* [[man]](https://man7.org/linux/man-pages/man2/listen.2.html)
   - *poll (2)* [[man]](https://man7.org/linux/man-pages/man2/poll.2.html)
   - *recv (2)* [[man]](https://man7.org/linux/man-pages/man2/recv.2.html)
   - *send (2)* [[man]](https://man7.org/linux/man-pages/man2/send.2.html)
   - *socket (2)* [[man]](https://man7.org/linux/man-pages/man2/socket.2.html)
   - *socket (7)* [[man]](https://man7.org/linux/man-pages/man7/socket.7.html)
   - *select (2)* [[man]](https://man7.org/linux/man-pages/man2/select.2.html)
- Brian “Beej Jorgensen” Hall, 2023, *Beej’s Guide to Network Programming Using Internet Sockets* [[beej.us]](https://beej.us/guide/bgnet/html/)
- Kurose, J. F., Ross, K. W., 2013, *Computer Networking: A Top Down Approach, Sixth Edition*, Chapter 1: Computer Networks and the Internet, pp. 1-82.
- Wikipedia, *Endianness* [[wikipedia.org]](https://en.wikipedia.org/wiki/Endianness)
- StackOverflow, *Why do we cast sockaddr_in to sockaddr when calling bind()?* [[stackoverflow.com]](https://stackoverflow.com/questions/21099041/why-do-we-cast-sockaddr-in-to-sockaddr-when-calling-bind)
- StackOverflow, *What are the differences between poll and select?* [[stackoverflow.com]](https://stackoverflow.com/questions/970979/what-are-the-differences-between-poll-and-select)

---
Source: [https://www.codequoi.com/en/sockets-and-network-programming-in-c/](https://www.codequoi.com/en/sockets-and-network-programming-in-c/)
