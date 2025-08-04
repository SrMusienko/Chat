#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <algorithm>

#pragma warning(disable: 4996)

struct TodoItem {
    int id;
    std::string description;
    bool completed;
};

std::vector<TodoItem> todoList;
std::recursive_mutex todoMutex;
int nextId = 1;

std::vector<SOCKET> clients;
std::recursive_mutex clientsMutex;

void broadcastList() {
    std::lock_guard<std::recursive_mutex> lock(todoMutex);
    std::ostringstream oss;
    for (const auto& item : todoList) {
        oss << item.id << " | " << item.description << " | " << (item.completed ? "Completed" : "Pending") << "\n";
    }
    std::string data = oss.str();

    std::lock_guard<std::recursive_mutex> lockClients(clientsMutex);
    for (auto client : clients) {
        send(client, data.c_str(), data.size(), 0);
    }
}

void handleClient(SOCKET clientSocket) {
    try {
        {
            std::lock_guard<std::recursive_mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        char buffer[1024];
        while (true) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) break;

            std::string command(buffer, bytesReceived);
            std::istringstream iss(command);
            std::string action;
            iss >> action;

            if (action == "ADD") {
                std::string description;
                std::getline(iss, description);
                description.erase(0, description.find_first_not_of(" ")); // Trim leading space

                std::lock_guard<std::recursive_mutex> lock(todoMutex);
                todoList.push_back({ nextId++, description, false });
                broadcastList();
            }
            else if (action == "TOGGLE") {
                int id;
                iss >> id;
                std::lock_guard<std::recursive_mutex> lock(todoMutex);
                for (auto& item : todoList) {
                    if (item.id == id) {
                        item.completed = !item.completed;
                        break;
                    }
                }
                broadcastList();
            }
            else if (action == "GET") {
                broadcastList();
            }
        }

        // Удалить клиента при отключении
        {
            std::lock_guard<std::recursive_mutex> lock(clientsMutex);
            clients.erase(std::remove(clients.begin(), clients.end(), clientSocket), clients.end());
        }
        closesocket(clientSocket);
    }
    catch (const std::exception& ex) {
        std::cerr << "Exception in thread: " << ex.what() << std::endl;
    }
}

int main() {
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        std::cout << "WSAStartup error\n";
        return 1;
    }

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, 0);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    std::cout << "Server started on 127.0.0.1:1111\n";

    while (true) {
        SOCKET newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
        if (newConnection != INVALID_SOCKET) {
            std::cout << "Client connected\n";
            std::thread([newConnection]() {
                handleClient(newConnection);
                }).detach();
        }
    }

    WSACleanup();
    return 0;
}