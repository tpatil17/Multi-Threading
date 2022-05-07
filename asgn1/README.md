#### Asignment1. 

This assignment implements three methods. 
GET, PUT, APPEND. 

GET: Reads any given file that has the right permissions and writes it to socket. 
If method was successful a message OK with status code 200 is sent to the server. 

PUT: Reads the given file or data, writes the it to the destination file.  
If destination file does not exist, a new file is created.  

If put created a new file, a status code 201 along with a message "created"  
is sent to the server. Else status code 200 and message "OK" is sent.  

APPEND: Reads from a given file or data and appends the contents to a supplied 
destination file. Destination file is given by the user.

If the Append method was successful, a message OK with status code 200 
is sent to the server. If the supplied file does not exist a message 
File Not Found with status code 404 is sent to the server.  


#### Errors. 

File Not Found 404, where 404 is the status code returned.  
This message is sent to the server when the file given by the user 
does not exist.  
Not applicable for PUT. 


Forbidden 403, 403 is the status code returned.  
This message and code is sent to the server if the file given by the user 
does not have the right permissions or not accessible.  
Or is a directory. 


Bad Request 400, 400 is the status code sent to the server along with 
message Bad Request when there is a syntactical problem in the Request. 
In other words the Request does not match the format.  

Internal Server Error 500, 500 is the status code, this error is sent when there is 
some problem in implementing the method because of an issue in the server like failing a write 
operation or read operation Or the version is missing or wrong.  

Not Implementd 501, 501 is the status code sent to the server when the request 
contains a method that is not put get or append. 

#### Programme. 

Based on the given starter code, handle_connection() first reads the socket into a buffer 
which is a stack, this buffer is then sent to a function process_request() which parses the string 
identifies the method, uri, version, header: value and checks for bad requests. 

After processing the function returns a Request Struct, which contains all the information 
like method, uri, version, content length and an offset. 
The offset is where the message body begins. 

Based on what the method is, function GET, PUT, or Append are called which implement 
methods get put and append.  

Get: takes in A response Struct A Request Struct and the socket file descriptor 
Then checks for the file's existance, permissions and then opens the file to read, based on 
the status of the file the function writes a response to the response Struct which is then 
written to the socket. 

Put: Takes in the initial buffer, Request Struct and Response Struct as arguments 
Then checks for errors, based on that writes to the response and writes the response. 
The function opens the given file, uses the buffer in argument to read the data file or data 
and writes to the file that was crated or opened. Static buffer is used which reads from socket 
and writes only from the offset so that only message or the data is written. 

Append: Similar to put takes in same arguments and the only differnce is that it appends to 
the destination file instead of over writing. Uses a static buffer to read the data from and write everything from 
offset.






