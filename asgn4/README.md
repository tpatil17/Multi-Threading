## ASGN4.  

Programme flow:  

The Programme is built on typing make.  
./httpserver -t [threads] -l [logfile] port starts the server with "threads" numeber of threads.  
The Programmeis capable of handeling PUT, GET and APPEND requests from the user. 
Multiple requests sent concorently are handled by using different threads that parallely run the Programme. 
The requests are stored in a queue data structure, which is built of a number array and is FIFO. 
The queue is used to implemet the thread pool design to assign requests to each thread, to avoid skipping requests
or overwriting them, a mutex lock is used while enqueing and dequeuing.  

As soon as threads are created they are waiting for a request to process, to avoid busy waiting the 
Programme uses conditonal variable which waits for a signal after enqueing. As soon the signal is recieved 
the thread in waiting is given a request.

To make sure that order of the requests is respected, while writing to the logfile or an input file
flock is used. This avoids overwriting and keeps the writng atomic.  

ERRORS:  

The Programme has bugs such as, when multiple requets are sent using olivertwist.py only the first request is 
recieved and completed. Multile requests on a bash script did work. The Programmeis capable of handeling a partially sent request.

