# socket-programming
Programs written in C to demonstrate how UNIX socket work and to create a server-client architecture using UNIX sockets.<br>

This repository contains three types of server-client architecture - <br>

# 1. Server - Client 1
  This server client architecture is a trivial one! <br>
  The server is a single process server which can maintain at most of one client connection at a time. Any connection request from other clients during a ongoing connection is
  rejected by the server. <br>

# 2. Server - client 2
  The server is a multiprocess server which spawns a new child process for each new connection request. Thus each connection is handled by different child processes. The server
  can handle multiple connections at a time. <br>
 
# 3. Server - client 3
  The server is a single process server which can handle multiple connections at a time upto a certain limit. In this case,the server can handle 256 connections at a time. Further
  Connections are rejected. The upper limit for the number of connections server can handle, can be changed by tweaking the server code a little bit.
