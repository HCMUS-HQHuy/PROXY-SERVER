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