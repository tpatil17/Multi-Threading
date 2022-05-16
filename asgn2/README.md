## ASGN2: Audit Log.  

Programme:  

The Programme has three structs: logbook, which contains the information that needs to be written to the log,  
struct Response, it store the information that is to be written to the socket as a reply to the socket,  
struct Request, stores the information harvested via the request recieved by the server,  

Function:  

process_rquest(): this Functiontakes in the socket file descriptor and the buffer that contains the request in order to parse. 
information like method, uri, version, content_length and request id is harvested from the buffer using strtok_r and sscanf. 
After the processing is done the function returns a variable of type struct Request which contains all the important information,  
that was give to the socket.  

enter_log(): this function takes in arguments (logbook, FILE) where the logbook contains the info to be written and FILE is the pointer to the log. 
Each entry is flushed as soon as it is written. This is to handle the sigterm and sigint commands.  

refresh_res():memsets' all buffers to zero also used to initialise struct Response.  
refresh_req():memsets' all buffers to zero also used to initialise struct Request.  

Get() implements get and returns a variable of type struct Response, in order to know the status code to be writtento the log.  
Put() implements put and returns a variable of type struct Response.
Append()  implements APPEND and returns a variable of type struct Response.

Flow:

handle_connection() is given with the socket discriptor connfd, this function then reads the socket and passes the buffer to process_rquest() to understand  what the request asks for, based on the method an operation is chose and performed, the performed operation is written immidiately to the audit log.  

This programme only uses static memory because it is faster than allocating a memory on the disk and then writing conentes to then fetch them back. Also in this situation since the we do not require to hold the information that we write stack allocation works the best.  
To be safe from unconditional jumps, memset(char arr[], 0, size), initialises the arr and prevents such errors.  

CAUTION: The programme expects to be given the requests in specific format or a bad request error will be thrown.  
In case of file not found the request will be logged in the file with a status code of 404. 
In case where we do not have the permissions to access the file a 403 status code will be logged in the audit log.  


