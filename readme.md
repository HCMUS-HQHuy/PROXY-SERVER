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