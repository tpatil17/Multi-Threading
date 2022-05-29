## ASGN3: Multi-threading. 

Programme:  

The Programme uses the same functions from previous assignments, where new additions are a int array[] requests,  
which stores connfd's of requests. A start_function() that starts all the threads and waits for requests. 
To signal the wait, the Programme uses a condition variable condQueue.  
In order to have safe distribution of tasks, mutex locks are used so that only one thread can access the array 
to enqueue or dequeue.  

Function:  

add_to_queue(): The function adds connfd (int) to an int array[] called requests and increases the count,  
which is protected by a mutex lock and uses a condition variable to signal that request was added.  
start_function(): This is called when the thread is created, the function is a while(1) loop that always waits for requests  
and upon getting one is signaled by a condition variable. Once the condition variable shows request is available the request is assigned to a thread. 
The dequeue is done under a mutex lock.  
Rest functions are the same excep fo the addition of the thread pool design.

Flow:  

Programme crates threads numebr of threads using pthread_create() and then threads wait for connection, once accept is called 
the add_to_queue is called to add the connfd to array[]. 
Once the connfd is added the condtion variable notifies thread to assign request that was added. 

ERRORS: On running the requests.toml file via oliver twist the first request is succesfully run and other requests are not run.  
