#include <windows.h>
#include <wininet.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "wininet.lib")


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void AddMessageToPostData(char* postData, const char* role, const char* content) {
    char newMessage[1024];

    // Sprawdzamy, czy w postData mamy tablicę "messages"
    if (strstr(postData, "\"messages\": [") != NULL) {
        // Znajdujemy miejsce, w którym kończy się tablica "messages" (przed ']')
        char* closingBracket = strchr(postData, ']');
        if (closingBracket != NULL) {
            // Tworzymy nową wiadomość w formacie JSON
            // Najpierw musimy zamienić wszystkie '\n' w 'content' na '\\n'
            char escapedContent[1024];
            int j = 0;

            // Przechodzimy przez każdy znak w 'content' i zamieniamy '\n' na '\\n'
            for (int i = 0; content[i] != '\0'; i++) {
                if (content[i] == '\n') {
                    escapedContent[j++] = '\\';
                    escapedContent[j++] = 'n';
                } else {
                    escapedContent[j++] = content[i];
                }
            }
            escapedContent[j] = '\0';  // Zakończenie ciągu

            // Tworzymy nową wiadomość, uwzględniając 'escapedContent'
            snprintf(newMessage, sizeof(newMessage),
                     ",{\"role\": \"%s\", \"content\": \"%s\"}", role, escapedContent);
            
            // Dodajemy nową wiadomość przed zamknięciem tablicy ']'
            size_t postDataLength = strlen(postData);
            size_t newMessageLength = strlen(newMessage);

            // Przesuwamy zawartość postData za miejscem, w którym ma być dodana nowa wiadomość
            memmove(closingBracket + newMessageLength, closingBracket, postDataLength - (closingBracket - postData) + 1);
            
            // Wstawiamy newMessage przed zamknięciem tablicy ']'
            memcpy(closingBracket, newMessage, newMessageLength);
        }
    }
}

// Funkcja do wysyłania żądania HTTP POST z danymi JSON
void SendHttpPostRequest(const char* url, char* postData) {
    HINTERNET hInternet, hConnect;
    HINTERNET hRequest;
    DWORD bytesRead;
    DWORD statusCode = 0;
    char buffer[4096];
    BOOL bResult;

    // Inicjalizowanie WinINet
    hInternet = InternetOpenA("HTTP Client", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        printf("Blad podczas otwierania polaczenia z Internetem: %ld\n", GetLastError());
        return;
    }

    // Tworzenie połączenia z serwerem
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
    char response[4096];
    memset(response, 0, sizeof(response));
    while (1) {
        bResult = InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead);
        if (!bResult || bytesRead == 0) {
            break;
        }
        strncat(response, buffer, bytesRead);
    }

    // Szukamy odpowiedzi AI w odpowiedzi JSON
    char* aiResponse = strstr(response, "\"content\":");
    if (aiResponse != NULL) {
        // Przesuwamy wskaźnik do treści odpowiedzi AI
        aiResponse += strlen("\"content\":");

        // Znajdź początek i koniec odpowiedzi
        char* start = strchr(aiResponse, '"');
        if (start != NULL) {
            start++; // Przesuwamy wskaźnik do pierwszego znaku odpowiedzi

            // Znajdź koniec odpowiedzi
            char* end = strchr(start, '"');
            if (end != NULL) {
                *end = '\0'; // Terminatorem ciągu jest znak '"'
                // Zamiana \n na prawdziwą nową linię
                char* pos = start;
                while ((pos = strstr(pos, "\\n")) != NULL) {
                    *pos = '\n';
                    memmove(pos + 1, pos + 2, strlen(pos + 2) + 1); // Przesunięcie reszty tekstu
                }

                // Wyświetlenie odpowiedzi AI
                printf("Odpowiedz AI: \n%s\n", start);

                AddMessageToPostData(postData, "assistant", start);
            }
        }
    } else {
        printf("Nie znaleziono odpowiedzi w JSON.\n");  // Jeśli odpowiedź nie zawiera 'content'
    }

    // Czyszczenie
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

int main() {
    const char* url = "http://localhost:1234/v1/chat/completions";  // Adres lokalnego serwera AI
    char userMessage[1024];
    char postData[16384];

    // Przygotowanie początkowego postData
    snprintf(postData, sizeof(postData),
        "{"
        "\"model\": \"llama-3.2-1b-instruct\","
        "\"messages\": ["
            "{\"role\": \"system\", \"content\": \"Always answer as short as you can.\"}"
        "],"
        "\"temperature\": 0.0"
        "}"); // Poczatek wiadomości (pusty zestaw danych)

    // Główna pętla do zadawania pytań
    while (1) {
        printf("Podaj wiadomosc uzytkownika (lub 'exit' aby zakonczyc): ");
        fgets(userMessage, sizeof(userMessage), stdin);
        userMessage[strcspn(userMessage, "\n")] = 0;  // Usunięcie znaku nowej linii

        // Jeśli użytkownik wpisze 'exit', kończymy pętlę
        if (strcmp(userMessage, "exit") == 0) {
            break;
        }

        // Dodanie wiadomości użytkownika do postData
        AddMessageToPostData(postData, "user", userMessage);

        // Wysyłanie żądania i wyświetlanie odpowiedzi po każdym zapytaniu
        SendHttpPostRequest(url, postData);
    }

    return 0;
}




