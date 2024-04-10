#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment (lib, "Ws2_32.lib")

using namespace std;

void initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        cerr << "WSAStartup failed: " << result << std::endl;
        exit(1);
    }
}

SOCKET connectToServer(const char* hostname, const char* port) {
    struct addrinfo hints, * res, * ptr;
    int result;
    SOCKET ConnectSocket = INVALID_SOCKET;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(hostname, port, &hints, &res);
    if (result != 0) {
        std::cerr << "getaddrinfo failed: " << result << std::endl;
        WSACleanup();
        exit(1);
    }

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            std::cerr << "Error at socket(): " << WSAGetLastError() << std::endl;
            freeaddrinfo(res);
            WSACleanup();
            exit(1);
        }

        // Connect to server.
        result = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (result == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    if (ConnectSocket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!" << std::endl;
        WSACleanup();
        exit(1);
    }

    return ConnectSocket;
}

void sendGETRequest(SOCKET ConnectSocket) {
    const char* httpRequest = "GET /webservices/FxRates/FxRates.asmx/getCurrentFxRates?tp=EU HTTP/1.1\r\n"
        "Host: www.lb.lt\r\n"
        "Connection: close\r\n\r\n";

    int result = send(ConnectSocket, httpRequest, (int)strlen(httpRequest), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        closesocket(ConnectSocket);
        WSACleanup();
        exit(1);
    }
}

void cleanup(SOCKET ConnectSocket) {
    closesocket(ConnectSocket);
    WSACleanup();
}

int main() {
    initializeWinsock();
    SOCKET ConnectSocket = connectToServer("www.lb.lt", "80");
    sendGETRequest(ConnectSocket);

    const int bufferSize = 512;
    char recvbuf[bufferSize];
    int result, recvbuflen = bufferSize;
    string response;

    do {
        result = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (result > 0)
            response.append(recvbuf, result);
        else if (result == 0)
            cout << "Connection closed\n";
        else
            cerr << "recv failed: " << WSAGetLastError() << endl;
    } while (result > 0);

    string userCurrency;
    while (true) {
        cout << "Enter the designation of the desired currency (e.g., USD), or type 'exit' to quit: ";
        cin >> userCurrency;

        if (userCurrency == "exit") {
            break;
        }

        size_t pos = response.find("<Ccy>" + userCurrency + "</Ccy>");
        if (pos != string::npos) {
            size_t ratePos = response.find("<Amt>", pos);
            string rate = response.substr(ratePos + 5, response.find("</Amt>", ratePos) - ratePos - 5);
            cout << "1 EUR = " << rate << " " << userCurrency << endl;
        }
        else {
            cout << "Currency not found or invalid input. Please try again." << endl;
        }
    }

    cleanup(ConnectSocket);
    return 0;
}