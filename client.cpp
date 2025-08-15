#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctime>
#include <string>

#define BUFFER_SIZE 1024

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <serv_ip> <serv_port>" << endl;
        return 1;
    }

    const char* serv_ip = argv[1];
    int serv_port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(serv_port);

    if (inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported." << endl;
        close(sock);
        return 1;
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Connection Failed." << endl;
        close(sock);
        return 1;
    }

    srand(time(0) + getpid());

    char buffer[BUFFER_SIZE];
    string prisoner_name;

    memset(buffer, 0, BUFFER_SIZE);
    int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        cerr << "Failed to receive prisoner name." << endl;
        close(sock);
        return 1;
    }
    prisoner_name = string(buffer);
    cout << "Assigned prisoner name: " << prisoner_name << endl;

    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        cerr << "Failed to receive welcome message." << endl;
        close(sock);
        return 1;
    }

    string welcome(buffer);
    cout << welcome << flush;

    size_t l_pos = welcome.find("between ");
    size_t r_pos = welcome.find(" and ", l_pos);
    size_t dot_pos = welcome.find(".", r_pos);

    int L = stoi(welcome.substr(l_pos + 8, r_pos - (l_pos + 8)));
    int R = stoi(welcome.substr(r_pos + 5, dot_pos - (r_pos + 5)));

    int low = L;
    int high = R;
    bool escaped = false;
    int attempt_count = 0;

    while (!escaped) {
        if (low > high) {
            cerr << "Invalid guess range" << endl;
            break;
        }

        int guess = low + rand() % (high - low + 1);
        string guess_str = to_string(guess);

        send(sock, guess_str.c_str(), guess_str.size(), 0);
        attempt_count++;

        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0) break;

        string response(buffer);

        if (response.find("Correct") != string::npos) {
            cout << "\nGuess " << guess << ": Correct You escaped!" << endl;
            escaped = true;
        }
        else if (response.find("too high") != string::npos) {
            cout << "Guess " << guess << ": Too high- New range: ["
                 << low << ", " << guess - 1 << "]" << endl;
            high = guess - 1;
        }
        else if (response.find("too low") != string::npos) {
            cout << "Guess " << guess << ": Too low | New range: ["
                 << guess + 1 << ", " << high << "]" << endl;
            low = guess + 1;
        }
        else if (response.find("out of attempts") != string::npos) {
            cout << "\nGuess " << guess << ": Wrong! You're out of attempts." << endl;
            cout << response;
            break;
        }
        else if (response.find("guessed the number") != string::npos) {
            cout << "\n" << response << flush;
        }

        sleep(1); 
    }

    close(sock);
    return 0;
}
