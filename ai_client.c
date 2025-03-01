//LM Studio
#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "wininet.lib")

// Funkcja do wysyłania żądania HTTP POST z danymi JSON
void SendHttpPostRequest(const char* url, const char* postData) {
    HINTERNET hInternet, hConnect;
    HINTERNET hRequest;
    DWORD bytesRead;
    char buffer[4096];
    BOOL bResult;
    
    // Inicjalizowanie WinINet
    hInternet = InternetOpenA("HTTP Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        printf("Blad podczas otwierania polaczenia z Internetem: %ld\n", GetLastError());
        return;
    }

    DWORD timeout = 120000;  // Czas oczekiwania ustawiony na 2 min
    InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));


    // Tworzenie połączenia z serwerem
    // Rozbijamy URL, aby uzyskać hosta i port
    const char* host = "localhost";
    const int port = 1234;

    hConnect = InternetConnectA(hInternet, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (hConnect == NULL) {
        printf("Blad podczas laczenia sie z serwerem: %ld\n", GetLastError());
        InternetCloseHandle(hInternet);
        return;
    }

    // Tworzenie żądania HTTP POST na URL /v1/chat/completions
    hRequest = HttpOpenRequestA(hConnect, "POST", "/v1/chat/completions", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
    if (hRequest == NULL) {
        printf("Blad podczas otwierania zadania POST: %ld\n", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    // Dodanie nagłówków
    const char* headers = "Content-Type: application/json\r\n";
    bResult = HttpAddRequestHeadersA(hRequest, headers, -1, HTTP_ADDREQ_FLAG_ADD);
    if (!bResult) {
        printf("Blad podczas dodawania naglowkow: %ld\n", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    // Wysłanie żądania HTTP POST z danymi JSON
    bResult = HttpSendRequestA(hRequest, NULL, 0, (LPVOID)postData, strlen(postData));
    if (!bResult) {
        printf("Blad podczas wysylania zadania: %ld\n", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    // Odczyt odpowiedzi z serwera
    while (1) {
        bResult = InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
        if (!bResult || bytesRead == 0) {
            break;
        }
        buffer[bytesRead] = '\0';  // Null-terminate the buffer
        printf("Odpowiedz: %s\n", buffer);  // Wyświetlanie odpowiedzi serwera
    }

    // Czyszczenie
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

int main() {
    const char* url = "http://localhost:1234/v1/chat/completions";  // Adres lokalnego serwera AI
    char userQuestion[1024];  // Bufor na pytanie użytkownika

    printf("Zadaj pytanie (wpisz 'exit' aby zakonczyc):\n");

    while (1) {
        // Pobieranie pytania od użytkownika
        printf("> ");
        fgets(userQuestion, sizeof(userQuestion), stdin);
        
        // Usuwamy nową linię na końcu (jeśli jest)
        userQuestion[strcspn(userQuestion, "\n")] = 0;

        // Jeśli użytkownik wpisał 'exit', kończymy program
        if (strcmp(userQuestion, "exit") == 0) {
            break;
        }

        // Przygotowanie danych JSON do wysłania w żądaniu POST
        char postData[2048];
        snprintf(postData, sizeof(postData),
            "{\n"
            "    \"model\": \"model-identifier\",\n"
            "    \"messages\": [\n"
            "        {\n"
            "            \"role\": \"system\",\n"
            "            \"content\": \"Always answer as short as you can.\"\n"
            "        },\n"
            "        {\n"
            "            \"role\": \"user\",\n"
            "            \"content\": \"%s\"\n"
            "        }\n"
            "    ],\n"
            "    \"temperature\": 0.7\n"
            "}", userQuestion);
        
        printf("Wysylanie zadania HTTP POST z danymi JSON...\n");
        SendHttpPostRequest(url, postData);
        
    }
    return 0;
}


