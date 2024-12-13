# OUR OWN PROXY
**Name:** DEMO PROXY SERVER.
> This is a project in second year in universtiy. Throughout the project, we want to demo the ability of a proxy server, include: transfer data, catching data, control request.

# Describe the project
## GOAL:
+ Lean about computer network, apply knowledges in university into real world.
+ Leaning how a computer can connect together.
+ 

# REQUIREMENT:
> Install: https://slproweb.com/products/Win32OpenSSL.html

# Important variables in code:

## localSocket:
* `localSocket` is main socket for proxy server. It is responsible for hearing from other sockets.
* In the project, localSocket is created and asigned with localhost and port. 
* localSocket listen and accept by `accept` and `clientSocket`.

## clientSocket:

* `clientSocket` is created by acception from localSocket. 
* It is used to get HTTP request from client (browser) and sent the response to client (browser).

## remoteSocket:
* `remoteSocket` is created to connect to server which is requested form client (browser).
* It is responsible for sending requests from clients and get data from the server.

# Current Process
* Create and listen connection from `localSocket`.
* Accept connection from client, create `clientSocket` to get HTTP request.
* Get information HOST of client and create remoteSocket to connect to server.
* Send request from client to server through out `remoteSocket`
* Get data from `remoteSocket` and send to `clientSocket`.

# Further note knowledge for this project
## WSAStartup: 
* `WSAStartup` initialize resource and config for winsock library, maintain version for system `MAKEWORD(2, 2)`.
* It is a notification for Windows and Windows contributes networking resource for software.
* If we ignore `WSAStartup`, functions `socket()`, `bind()`, `listen()`, `accept()` will be failed
* **IMPORTANT!!**: Free Winsock when the programe finished (`WSACleanup`).
## select:
### The macros to manipulate and check fd+set contents:
* *FD_ZERO(\*set)*: initialize set to the empty set. A set should always be cleared before using.
* *FD_CLS(s, \*set)*`: removes socket from set.
* *FD_ISSET(s, \*set)*: checks to se if `s` is a membet of set and returns TRUE if so.
* *FD_SET(s, \*set)*: adds socket `s` to set.
After run `select` a while, the readfds will remove sockets that are not activated. 


## SOME NOTES:
* 10053 ERRORs with clientSocket: it is errors when client sends a short message without waiting responding. So if select wait for a long time. This may occur error closed server from browser.
### Vấn đề hiện tại: 
* Việc giữ CONNECT cho một trang web là tốt hay xấu? Nếu nhiều tab được browser mở lên mà mỗi tab điều là nội dung của trang web youtube.com thì nó có được gửi lại lệnh CONNECT cho mỗi tab không? 
* Làm thế nào để ngắt PROXY hiệu quả?
* Tối ưu lệnh connect cho mỗi client.

# IMPROVEMENT. (WAITING ...)
## ProxyServer Class: 
Manages the overall functionality of the proxy server, including handling client requests and forwarding them to the destination server. \
### Attributes:
	* host: Proxy server host (IP address).
	* port: Listening port for client connections.
	* cache: Instance of Cache (optional).
	* logger: Instance of Logger (optional).
### Methods:
	* start(): Starts the server and listens for incoming connections.
	* accept_client(): Accepts a new client connection and spawns a ClientHandler.
	* stop(): Stops the server gracefully.

## ClientHandler Class: 
Handles individual client connections, processes requests, and manages communication between the client and the proxy server.
### Attributes:
	* client_socket: Socket representing the client connection.
	* request: Instance of Request.
	* response: Instance of Response.
	* proxy_server: Reference to the ProxyServer.
### Methods:
	* handle_request(): Reads data from the client, processes it, and sends it to the destination server.
	* forward_request(): Forwards the processed request to the actual server.
	* send_response(): Sends the response back to the client after processing.

## Request Class: 
Represents a client request. This class parses and stores request details (such as headers, request type, and URL) and might provide utility functions for modification.
### Attributes:
	* method: HTTP method (GET, POST, etc.).
	* url: Target URL.
	* headers: Request headers.
	* body: Request body (for POST requests).
### Methods:
	* parse(): Parses raw request data from the client.
	* modify_request(): Modifies headers or body if needed (e.g., for adding headers).

## Response Class: 
Represents the response from the destination server, including methods for reading, storing, and modifying response data as needed.
### Attributes:
	* status_code: HTTP status code (e.g., 200, 404).
	* headers: Response headers.
	* body: Response body (HTML, JSON, etc.).
### Methods:
	* parse(): Parses raw response data from the destination server.
	* modify_response(): Modifies headers or body if needed.


Cache Class (Optional):Stores responses for certain requests to improve performance by serving cached data instead of forwarding every request.
Attributes:
	cache_data: Dictionary or other structure to store cached responses.
Methods:
	get_cached_response(request): Returns cached response if available.
	store_response(request, response): Stores a response in the cache.


Logger Class (Optional): Logs activity for debugging, performance tracking, and security monitoring.
Attributes:
	log_file: Path to the log file.
Methods:
	log(message): Writes messages to a log file.

??? How to block ads when access a website.



