#include <iostream>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 8080
#define MAX_ATTEMPTS 15

std::mutex cout_mutex;
std::mutex clients_mutex;
std::mutex winners_mutex;

int client_sockets[4] = {0};
std::string winners[4];
int winner_count = 0;

int genRandom(int min, int max) {
    return rand() % (max - min + 1) + min;
}

void broadcast_message(const std::string& msg) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (int i = 0; i < 4; ++i) {
        if (client_sockets[i] != 0) {
            send(client_sockets[i], msg.c_str(), msg.size(), 0);
        }
    }
}

void control_client(int clientfd, int clientId, int L , int R , int X) {
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_sockets[clientId - 1] = clientfd;
    }

    char buffer[1024];
    int attempts = 0;

    std::string welcomeMsg = "Welcome, Prisoner " + std::to_string(clientId) + "!\n";
    welcomeMsg += "Guess the number between " + std::to_string(L) + " and " + std::to_string(R) + ".\n";
    welcomeMsg += "You have " + std::to_string(MAX_ATTEMPTS) + " attempts.\n";
    send(clientfd, welcomeMsg.c_str(), welcomeMsg.size(), 0);

    while (attempts < MAX_ATTEMPTS) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientfd, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break;

        int Y = atoi(buffer);
        std::string response;

        if (Y == X) {
            response = "Correct. You guessed the number\n";
            send(clientfd, response.c_str(), response.size(), 0);
            {
                std::lock_guard<std::mutex> lock(winners_mutex);
                winners[winner_count++] = "Prisoner " + std::to_string(clientId);

                std::string broadcast_msg = ">>> Prisoner " + std::to_string(clientId) + " guessed the number\n";
                broadcast_msg += " Winners till now:\n";
                for (int i = 0; i < winner_count; ++i) {
                    broadcast_msg += std::to_string(i + 1) +  winners[i] + "\n";
                }
                broadcast_message(broadcast_msg);
            }
            break;
        } else if (Y < X) {
            response = "Value too low\n";
        } else {
            response = "Value too high\n";
        }

        attempts++;
        if (attempts == MAX_ATTEMPTS) {
            response += "You are out of attempts. The  number was " + std::to_string(X) + "\n";
        }

        send(clientfd, response.c_str(), response.size(), 0);
    }

    close(clientfd);

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        client_sockets[clientId - 1] = 0;
    }

    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "Client " << clientId << " disconnected\n";
}

int main() {
    srand(time(NULL));

    int L = genRandom(1, 100000);
    int diff = genRandom(10001, 99999); 
    int R = L + diff;
    int X = genRandom(L, R);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serv_addr;
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Bind failed.\n";
        return 1;
    }

    if (listen(sockfd, 4) < 0) {
        std::cerr << "Listen failed.\n";
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << "\n";

    int clientId = 1;
    while (clientId <= 4) {
        sockaddr_in client_addr;
        int addrLen = sizeof(client_addr);
        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, (socklen_t*)&addrLen);
        if (clientfd < 0) {
            std::cerr << "Accept failed\n";
            continue;
        }

        std::thread(control_client, clientfd, clientId, L , R , X).detach();
        clientId++;
    }

    while (true) {
        {
            std::lock_guard<std::mutex> lock(winners_mutex);
            if (winner_count == 4) break;
        }
        sleep(1);
    }

    std::cout << "Escape Order:\n";
    for (int i = 0; i < 4; ++i) {
        std::cout << i + 1 << ". " << winners[i] << "\n";
    }

    close(sockfd);
    return 0;
}
