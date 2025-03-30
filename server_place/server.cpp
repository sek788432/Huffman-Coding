#include "../utils.h"

// Base class for network operations
class NetworkBase {
   protected:
    int sockfd{-1};
    struct sockaddr_in addr;
    std::string ip;
    int port;

    NetworkBase(std::string_view ip, int port) : ip(ip), port(port) {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.data());
    }

    virtual ~NetworkBase() {
        if (sockfd >= 0) {
            close(sockfd);
        }
    }

    void setSocketOptions() {
        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) <
            0) {
            throw std::runtime_error("Failed to set socket options");
        }
    }
};

// Huffman decoder class
class HuffmanDecoder {
   private:
    std::map<unsigned char, std::string> codeMap;

    static std::string toBinary(unsigned char a) {
        std::string output;
        output.reserve(8);
        for (int i = 7; i >= 0; --i) {
            output += (a & (1 << i)) ? "1" : "0";
        }
        return output;
    }

   public:
    void readCodeMap(const std::string& filename) {
        std::ifstream fin(filename);
        if (!fin) {
            throw std::runtime_error("Failed to open code file: " + filename);
        }

        std::string line;
        while (std::getline(fin, line)) {
            if (line.empty()) continue;
            unsigned char key = line[0];
            size_t colonPos = line.find(':');
            if (colonPos == std::string::npos) {
                throw std::runtime_error("Invalid code format");
            }
            codeMap[key] = line.substr(colonPos + 1);
        }
    }

    void decompressFile(const std::string& inputPath,
                        const std::string& outputPath) {
        std::ifstream input(inputPath, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Failed to open input file: " + inputPath);
        }

        // Read header
        int paddedBits;
        input.read(reinterpret_cast<char*>(&paddedBits), sizeof(paddedBits));

        int codeMapSize;
        input.read(reinterpret_cast<char*>(&codeMapSize), sizeof(codeMapSize));

        // Read code map
        for (int i = 0; i < codeMapSize; ++i) {
            unsigned char key;
            input.read(reinterpret_cast<char*>(&key), sizeof(key));

            int valueLength;
            input.read(reinterpret_cast<char*>(&valueLength),
                       sizeof(valueLength));

            std::string value(valueLength, '\0');
            input.read(&value[0], valueLength);

            codeMap[key] = value;
        }

        // Read and process data
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input),
                                          {});
        std::string bitString;
        bitString.reserve(buffer.size() * 8);

        for (unsigned char byte : buffer) {
            bitString += toBinary(byte);
        }

        // Remove padding
        bitString = bitString.substr(0, bitString.length() - paddedBits);

        // Decode
        std::vector<unsigned char> decodedData;
        std::string currentCode;

        for (char bit : bitString) {
            currentCode += bit;
            for (const auto& [key, code] : codeMap) {
                if (currentCode == code) {
                    decodedData.push_back(key);
                    currentCode.clear();
                    break;
                }
            }
        }

        // Write output
        std::ofstream output(outputPath, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Failed to open output file: " +
                                     outputPath);
        }
        output.write(reinterpret_cast<const char*>(decodedData.data()),
                     decodedData.size());
    }
};

// Server class
class Server : public NetworkBase {
   private:
    std::unique_ptr<HuffmanDecoder> decoder;
    std::vector<std::string> filePaths;

    void handleClient(int clientSocket, const std::string& filePath) {
        try {
            std::ofstream file(filePath, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open file for writing: " +
                                         filePath);
            }

            std::vector<char> buffer(BUFFER_SIZE);
            ssize_t totalBytes = 0;

            while (true) {
                ssize_t bytesReceived =
                    recv(clientSocket, buffer.data(), buffer.size(), 0);
                if (bytesReceived < 0) {
                    throw std::runtime_error("Network error during receive");
                }
                if (bytesReceived == 0) break;

                file.write(buffer.data(), bytesReceived);
                totalBytes += bytesReceived;
            }

            Logger::info("Received %zd bytes for file: %s", totalBytes,
                         filePath.c_str());
        } catch (const std::exception& e) {
            Logger::error("Error handling client: %s", e.what());
            throw;
        }
    }

   public:
    Server() : NetworkBase(SERVER_IP.data(), SERVER_PORT) {
        decoder = std::make_unique<HuffmanDecoder>();
        filePaths = {COMPRESSED_FILE.data(), CODE_FILE.data(),
                     FOR_DECOMPRESSED_FILE.data(), COMPRESSED_FILE.data()};
    }

    void start() {
        try {
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                throw std::runtime_error("Failed to create socket");
            }

            setSocketOptions();

            if (::bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr),
                       sizeof(addr)) < 0) {
                throw std::runtime_error("Failed to bind socket");
            }

            if (listen(sockfd, 10) < 0) {
                throw std::runtime_error("Failed to listen on socket");
            }

            Logger::info("Server listening on %s:%d", SERVER_IP.data(),
                         SERVER_PORT);

            for (size_t i = 0; i < filePaths.size(); ++i) {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);

                int clientSocket = accept(
                    sockfd, reinterpret_cast<struct sockaddr*>(&clientAddr),
                    &clientAddrLen);
                if (clientSocket < 0) {
                    throw std::runtime_error("Failed to accept connection");
                }

                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP,
                          sizeof(clientIP));
                int clientPort = ntohs(clientAddr.sin_port);

                Logger::info("Accepted connection from %s:%d", clientIP,
                             clientPort);

                pid_t pid = fork();
                if (pid < 0) {
                    throw std::runtime_error("Failed to fork process");
                }

                if (pid == 0) {  // Child process
                    close(sockfd);
                    handleClient(clientSocket, filePaths[i]);
                    close(clientSocket);
                    exit(0);
                } else {  // Parent process
                    close(clientSocket);
                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                        Logger::error("Client process failed with status %d",
                                      WEXITSTATUS(status));
                    }
                }
            }

            // Process received files
            decoder->readCodeMap(FOR_DECOMPRESSED_FILE.data());
            decoder->decompressFile(COMPRESSED_FILE.data(),
                                    DECODED_FILE.data());
            Logger::info("File decompression completed successfully");

        } catch (const std::exception& e) {
            Logger::critical("Fatal error: %s", e.what());
            throw;
        }
    }
};

int main() {
    try {
        Logger::setLogFileName("server.log");
        Server server;
        server.start();
    } catch (const std::exception& e) {
        Logger::critical("Fatal error: %s", e.what());
        return 1;
    }
    Logger::close();
    return 0;
}
