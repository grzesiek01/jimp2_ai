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
        printf("Błąd podczas otwierania połączenia z Internetem: %ld\n", GetLastError());
        return;
    }

    // Tworzenie połączenia z serwerem
    // Rozbijamy URL, aby uzyskać hosta i port
    const char* host = "localhost";
    const int port = 1234;

    hConnect = InternetConnectA(hInternet, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (hConnect == NULL) {
        printf("Błąd podczas łączenia się z serwerem: %ld\n", GetLastError());
        InternetCloseHandle(hInternet);
        return;
    }

    // Tworzenie żądania HTTP POST na URL /v1/chat/completions
    hRequest = HttpOpenRequestA(hConnect, "POST", "/v1/chat/completions", NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);
    if (hRequest == NULL) {
        printf("Błąd podczas otwierania żądania POST: %ld\n", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    // Dodanie nagłówków
    const char* headers = "Content-Type: application/json\r\n";
    bResult = HttpAddRequestHeadersA(hRequest, headers, -1, HTTP_ADDREQ_FLAG_ADD);
    if (!bResult) {
        printf("Błąd podczas dodawania nagłówków: %ld\n", GetLastError());
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    // Wysłanie żądania HTTP POST z danymi JSON
    bResult = HttpSendRequestA(hRequest, NULL, 0, (LPVOID)postData, strlen(postData));
    if (!bResult) {
        printf("Błąd podczas wysyłania żądania: %ld\n", GetLastError());
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
        printf("Odpowiedź: %s\n", buffer);  // Wyświetlanie odpowiedzi serwera
    }

    // Czyszczenie
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

int main() {
    const char* url = "http://localhost:1234/v1/chat/completions";  // Adres lokalnego serwera AI

    // Przygotowanie danych JSON do wysłania w żądaniu POST
    const char* postData = 
        "{"
        "\"model\": \"model-identifier\","
        "\"messages\": ["
            "{\"role\": \"system\", \"content\": \"Always answer in rhymes.\"},"
            "{\"role\": \"user\", \"content\": \"Introduce yourself.\"}"
        "],"
        "\"temperature\": 0.7"
        "}";

    printf("Wysyłanie żądania HTTP POST z danymi JSON do URL: %s\n", url);
    SendHttpPostRequest(url, postData);
    return 0;
}


