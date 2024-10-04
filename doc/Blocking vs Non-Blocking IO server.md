# Blocking vs Non-Blocking I/O server, Concurrency, Parallelism

Servers have to do I/O tasks like reading data from disk or memory, sending and receiving data from databases over the network. I/O tasks are the ones where the CPU can’t do it by itself. It relies on other components which involve a long period of waiting. In the meantime, the CPU switches to other tasks while waiting. In this way even when the server is waiting for data from the database, other tasks can move on. Some I/O requests are blocked, which means control does not return to the application until the I/O task is over. In such cases, the CPU has to run multiple threads/processes to complete other tasks. Before diving into blocking and non-blocking I/O let's go through some fundamentals of operating systems.

## Concurrency and Parallelism
The Dictionary meaning of concurrency and parallelism is almost similar but in computer science, the two words mean different things. In the operating system, concurrency means Interruptibilty and parallelism means Independentability.

This can be explained with an example. Let's say you have two tasks: get a passport from the passport center and finish the assignment. Say passport task takes 3 hours in which you apply, wait for your turn to have your photo taken and get a receipt. An assignment takes 3 hours which requires you to study the course work, solve the problems and format it. In total it takes 6 hours to complete two tasks. But if you take your laptop to the passport center and study while waiting for your turn then after you are done with the passport you spend only time solving problems and formatting assignments. So you interrupted your passport task while waiting and partly did your assignment. This is Concurrency. But for security reasons you need to give away all electronic devices so you can’t use your laptop to study. If you take help from a friend and ask him to solve half problems from the assignment while you are doing your passport task then after your passport task you can solve the remaining half. In this way you both tasks were carried out with depending on one another this is Parallelism.

![Concurrency vs Parallelism](https://coblob-publish.s3.us-west-2.amazonaws.com/images/concurency-parallelism-61b662dbef1ba70d59a93efc.jpg)

### Definition of Concurrency and Parallelism
Concurrency means executing multiple tasks at the same time but not necessarily simultaneously. In a concurrent application, two tasks can start, run, and complete in overlapping periods. Parallelism means performing two or more tasks simultaneously. In concurrency a single CPU core can do multiple tasks by context-switching while in parallelism a CPU needs to run multiple processes/threads to do multiple tasks.

## Process and Thread

In an operating system a process has its memory and call stack and does not share its resource with other processes and communicate with other process over IPC(Inter Process Communication). Multiprocessing runs several processes and requires multiple CPUs.

Process spawn multiple threads to run multiple code segments. Unlike process threads can share heap, address space. A single CPU with single process running is enough to run multiple threads. Therefore concurrency can be achieved by using multithreading while parallelism is achieved by multiprocessing in application.

![Process and Thread](https://coblob-publish.s3.us-west-2.amazonaws.com/images/Procees-and-thread-61b662dbef1ba70d59a93efc.jpg)

### Context switching in multiprocessing and multithreading
Context-switching involves switching of CPU from one task to another task. Context switching in multiprocess is slower than multithreaded application. This is because before switching to a new process os kernel has to cache the current process using Translation Lookaside Buffer (TLB). This is not required in multithreading as threads share the same address space.

### When to use multiprocessing?
If you have a multi-core CPU and you want to achieve true parallelism, use multiprocessing in your application. Applications that do very complex operations and require a lot of data are ideal for multiprocessing. Ex AI application training neural network. In addition to that if your application is not I/O bound that requires loading data from external disk or over network. Multiprocessing is also used if you want to achieve reliability.

### When to use multithreading?
Multiprocess applications have a higher memory footprint as processes don't share memory. If an application has to do more I/O tasks, multithreading is a good option. Multithreading applications are lightweight, less memory intensive and highly concurrent. With asynchronous threads one can achieve true concurrency. Multithreading is used in backend servers, web browsers, web servers etc.

## Synchronous vs Asynchronous
In the above example let say you now have to write two assignments. You can write only one assignment at a time unless you can write with both hands(ambidextrous). Thus tasks have to be synchronised one after another. While we can perform two tasks of writing assignment and going to the passport office, like explained before, they are asynchronous. As we complete another task while waiting.

![Synchronous and Asynchronous](https://coblob-publish.s3.us-west-2.amazonaws.com/images/sync-vs-async-61b662dbef1ba70d59a93efc.jpg)

Synchronization is required when threads share the same resources. Kernel needs to lock(mutex) the resource as one thread is processing. If multiple threads access the same resource it may cause errors or even deadlocks. So to reduce such issues the kernel synchronizes the tasks and performs one after another. In case of asynchronous tasks multiple threads can take part and finish jobs asynchronously.


## Blocking and Non Blocking I/O

### Blocking I/O Servers
Now that we have gone through the concept of concurrency and multithreading it is easier to understand Blocking I/O servers. In blocking I/O, server is allocated a pool of threads to handle incoming requests. Threads are assigned until requests are fully served like in synchronous tasks. In majority cases the CPU is ideal just waiting for requests to complete. As soon as a request arrives a thread has to read the socket, make another I/O call to access data from DB and write the data to the socket. Because it is synchronized it has to wait for each I/O call to finish. This makes thread slow due to context-switching, requires more memory and utilizes other hardware resources inefficiently. Due to this inefficiency it is easily prone to DOS(denial of service) attacks. Older versions of Tomcat, Apache Web Server and servlets use this method.

### Non-Blocking I/O Servers
In non-blocking I/O single thread serves multiple requests by using non-blocking kernel I/O features. When a request is received it reads from socket and makes I/O calls to get data from DB. While it is waiting for data it starts processing another request. As soon as data is available it will context switch to write the data to socket while waiting for data from DB for second request. These requests are processed asynchronously based on number requests and availability of hardware and CPU. Because no requests are blocked it is called non-blocking I/O servers. This is done through an event loop which handles interrupts from I/O. Netty, Webflux, nginx, servlet 3.1+, Node, Go are popular non-blocking Webservers.

### Trade-offs
- Non-Blocking I/O servers are more complex to design and implement than Blocking I/O servers. Code is more complex and hard to debug. In case of bugs it is hard to reproduce as we might not be able to capture the exact state of the server. Writing unit tests is complex and might not achieve 100% code coverage.
- As Non-Blocking I/O runs on a single thread it might not be truly parallel and performance might degrade if given more CPU intensive tasks. Though it is suitable for applications which are less memory intensive. Blocking I/O are good more CPU intensive tasks as multiple threads can run parallel on other CPUs. It suitable to use for memory intensive tasks.
- Non-Blocking I/O uses resources efficiently and is perfect for scalable applications. While Blocking I/O doesn't use resources efficiently it is less prone to errors. And is good candidate when reliability is more important than scalability. Most of ACID requests are handled by Blocking I/O.
- Non-Blocking are less prone to DOS attacks than blocking I/O.

___

Source: [Blocking vs Non-Blocking I/O server, How to choose? Concurrency, Parallelism and many more](https://coblob.com/blogs/Blocking-vs-Non-Blocking-IO-server-How-to-choose-Concurrency-Parallelism-and-many-more-61b662dbef1ba70d59a93efc)

Also see: [Overview of Blocking vs Non-Blocking in Node.js](https://nodejs.org/en/learn/asynchronous-work/overview-of-blocking-vs-non-blocking)
