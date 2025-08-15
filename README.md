Multithreaded Client-Server TCP Socket Game:
A multithreaded C++ client–server number-guessing game using TCP sockets.
The server generates a random range [L, R] and a secret number X, accepts up to 4 concurrent clients, and replies to each guess with too low / too high / correct. On a correct guess, it broadcasts to all clients and records the escape order. Shared state (client list, winners) is protected with mutexes.

Features:
TCP socket programming in C++ .
Multithreaded server (one thread per client).
Up to 4 clients guessing simultaneously.
Broadcast announcements when someone guesses correctly.
Winner tracking: final escape order printed by server.
Attempt limit (configurable) and enforced range size (≥ 10⁴).

Tech Stack:
C++17, STL (<thread>, <mutex>)
POSIX Sockets (<sys/socket.h>, <netinet/in.h>, <arpa/inet.h>)
Tested on Linux(Windows users: run via WSL)

How It Works:
Server boot: Generates L, R (with R-L ≥ 10^4) and a secret X ∈ [L, R].
Connections: Accepts up to 4 clients and spawns a thread per client.
Gameplay:
  -Client sends integer guesses.
  -Server responds: "Value too low\n", "Value too high\n", or "Correct. You guessed the number\n".
  -On correct guess, server broadcasts to all clients and records the winner.
Finish: After all winners determined (or attempts exhausted), server prints the escape order.
