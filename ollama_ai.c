//Ollama
#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "wininet.lib")

#define HISTORY_LEN 10   // Maksymalna liczba przechowywanych wiadomości
#define MAX_MSG_LEN 1024  // Maksymalna długość pojedynczej wiadomości


char history[HISTORY_LEN][MAX_MSG_LEN]; // Historia rozmowy
int historyCount = 0;  // Licznik przechowywanych wiadomości

// Dodaje wiadomość do historii, przesuwając starsze wiadomości
void addToHistory(const char* content) {
    if (historyCount >= HISTORY_LEN) {
        // Jeśli przekroczyliśmy limit, usuwamy najstarszą wiadomość
        for (int i = 1; i < HISTORY_LEN; i++) {
            strncpy(history[i - 1], history[i], sizeof(history[i]));
        }
        historyCount--;
    }
    
    // Dodajemy nową wiadomość na koniec
    strncpy(history[historyCount], content, sizeof(history[historyCount]) - 1);
    historyCount++;
}

// Tworzy JSON z pełnym kontekstem rozmowy
void buildJsonPayload(char* postData, size_t size, const char* lastQuestion) {
    snprintf(postData, size, "{\n    \"model\": \"phi4:latest\",\n    \"messages\": [\n");

    // Tworzymy podsumowanie historii rozmowy
    strncat(postData, "        {\"role\": \"system\", \"content\": \"Answer as short as you can. Context of the conversation:\\n", size - strlen(postData) - 1);

    // Dodajemy historię rozmowy jako tekst w "system"
    for (int i = 0; i < historyCount; i++) {
        snprintf(postData + strlen(postData), size - strlen(postData),
                 "user: %s\\n",
                 history[i]);
    }

    // Zamykamy historię
    strncat(postData, "\"},\n", size - strlen(postData) - 1);

    // Dodajemy ostatnie pytanie użytkownika jako normalną wiadomość
    snprintf(postData + strlen(postData), size - strlen(postData),
             "        {\"role\": \"user\", \"content\": \"%s\"}\n",
             lastQuestion);

    strncat(postData, "    ],\n    \"temperature\": 0.7\n}", size - strlen(postData) - 1);
}


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
    const char* host = "127.0.0.1";
    const int port = 11434;

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
    const char* url = "http://127.0.0.1:11434";  // Adres lokalnego serwera AI
    char userQuestion[MAX_MSG_LEN];

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

        char postData[8192];
    
        if (strcmp(userQuestion, "exit") == 0) {
            break;
        }
    
        // Dodaj pytanie do historii rozmowy
        addToHistory(userQuestion);
    
        // Zbuduj JSON-a uwzględniającego całą historię rozmowy
        buildJsonPayload(postData, sizeof(postData), userQuestion);
        printf("Wysylanie zadania HTTP POST z danymi JSON...\n");
        SendHttpPostRequest(url, postData);
        
    }
    return 0;
}


