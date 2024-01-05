#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <fcntl.h>
using namespace std;

SSL_CTX* initSSLContext() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    SSL_CTX* sslContext = SSL_CTX_new(TLS_client_method());
    if (!sslContext) {
        cerr << "Error creating SSL context" << endl;
        return nullptr;
    }

    return sslContext;
}

string base64_encode(const unsigned char* input, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, input, static_cast<int>(length));
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bufferPtr);

    string result(bufferPtr->data, bufferPtr->length - 1);  // Exclude newline
    BIO_free_all(bio);

    return result;
}

string generateWebSocketKey() {
    unsigned char nonce[16];
    RAND_bytes(nonce, sizeof(nonce));
    return base64_encode(nonce, sizeof(nonce));
}

int main() {
    // Initialize SSL
    SSL_CTX* sslContext = initSSLContext();
    if (!sslContext) {
        return -1;
    }

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Error creating socket" << endl;
        SSL_CTX_free(sslContext);
        return -1;
    }
    // Resolve the domain name to an IP address
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; 
    if (getaddrinfo("fstream.binance.com", nullptr, &hints, &result) != 0) {
        cerr << "Error resolving domain name" << endl;
        close(clientSocket);
        SSL_CTX_free(sslContext);
        return -1;
    }

    // Set up the server address structure
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(443);
    serverAddress.sin_addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr;

    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Error connecting to the server" << endl;
        close(clientSocket);
        SSL_CTX_free(sslContext);
        return -1;
    }

    // Initialize SSL on the socket
    SSL* ssl = SSL_new(sslContext);
    if (!ssl) {
        cerr << "Error creating SSL structure" << endl;
        close(clientSocket);
        SSL_CTX_free(sslContext);
        return -1;
    }

    SSL_set_fd(ssl, clientSocket);
    if (SSL_connect(ssl) <= 0) {
        cerr << "Error establishing SSL connection" << endl;
        close(clientSocket);
        SSL_free(ssl);
        SSL_CTX_free(sslContext);
        return -1;
    }

    // Free the result of getaddrinfo
    freeaddrinfo(result);

    const string webSocketKey = generateWebSocketKey();
    // Create the WebSocket handshake request with the correct HTTPS scheme
    const string request = "GET /ws HTTP/1.1\r\n"
                          "Host: fstream.binance.com\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Key: " + webSocketKey + "\r\n"
                          "Sec-WebSocket-Version: 13\r\n"
                          "\r\n";

    // Send the WebSocket handshake request
    SSL_write(ssl, request.c_str(), request.size());

    char response[4096];
    memset(response, 0, sizeof(response));
    SSL_read(ssl, response, sizeof(response));
    cout << "Server Response:\n" << response << endl;

    const char* subscriptionMessage = " { \"method\": \"SUBSCRIBE\", \"params\": [ \"bnbusdt@aggTrade\" ], \"id\": 45 } ";


    if(SSL_write(ssl, subscriptionMessage, strlen(subscriptionMessage)) <= 0)
    {
        cerr<<"Error sending subscription message"<<endl;
        return -1;

    }
    cout << "Sent Subscription Message:\n" << subscriptionMessage << endl;

    int bytesRead;

    do {
        memset(response, 0, sizeof(response));
        bytesRead = SSL_read(ssl, response, sizeof(response));
        if (bytesRead > 0) {
            cout << "Received WebSocket Data:\n" << response << endl;
            // Check if this is the end of the WebSocket frame or message
            if (strstr(response, "\r\n\r\n") != nullptr) {
                break;
            }
        } else if (bytesRead == 0) {
            cerr << "Still reading" << endl;
            continue;
        } else {
            cerr << "Error reading WebSocket data." << endl;
            break;
        }
    } while (true);

    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(sslContext);
    close(clientSocket);

    return 0;
}