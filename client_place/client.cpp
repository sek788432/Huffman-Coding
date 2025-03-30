#include "../utils.h"

// Huffman Tree Node
struct TreeNode {
    int frequency;
    unsigned char key;
    TreeNode* left;
    TreeNode* right;
    TreeNode(unsigned char x, int y)
        : frequency(y), key(x), left(nullptr), right(nullptr) {}
};

// Comparator for priority queue
struct Comp {
    bool operator()(TreeNode* a, TreeNode* b) {
        if (a->frequency != b->frequency) return a->frequency > b->frequency;
        return a->key > b->key;
    }
};

// Huffman Compressor class
class HuffmanCompressor {
   private:
    std::map<unsigned char, std::string> codeMap;
    std::string writeToFile;

    static std::string toBinary(unsigned char a) {
        std::string output;
        output.reserve(8);
        for (int i = 7; i >= 0; --i) {
            output += (a & (1 << i)) ? "1" : "0";
        }
        return output;
    }

    void treeTraversal(TreeNode* root, std::string current,
                       long long int& sum) {
        if (root->left) treeTraversal(root->left, current + "0", sum);
        if (!root->left && !root->right) {  // leaf node
            std::string tmp(1, root->key);
            writeToFile += tmp + " -> " + std::to_string(root->frequency) +
                           "(" +
                           std::to_string((float)root->frequency * 100 / sum) +
                           "%)" + " -> " + current + "\n";
            codeMap[root->key] = current;
        }
        if (root->right) treeTraversal(root->right, current + "1", sum);
    }

    std::vector<unsigned char> getBufferFromString(const std::string& bitstring,
                                                   int& sz) {
        std::vector<unsigned char> outputBuffer;
        int interval = 0;
        unsigned char bit = 0;

        for (int i = 0; i < sz; i++) {
            bit = (bit << 1) | (bitstring[i] - '0');
            interval++;
            if (interval == 8) {
                interval = 0;
                outputBuffer.push_back(bit);
                bit = 0;
            }
        }

        sz = outputBuffer.size();
        return outputBuffer;
    }

    void writeFileFromBuffer(const std::string& path,
                             const std::vector<unsigned char>& buffer,
                             bool append = false) {
        std::ofstream file(path, std::ios::binary | (append ? std::ios::app
                                                            : std::ios::trunc));
        if (!file) {
            throw std::runtime_error("Failed to open file for writing: " +
                                     path);
        }
        file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }

    void writeHeader(const std::string& path, int paddedBits) {
        int size = codeMap.size();
        writeFileFromBuffer(
            path,
            std::vector<unsigned char>(
                reinterpret_cast<unsigned char*>(&paddedBits),
                reinterpret_cast<unsigned char*>(&paddedBits) + sizeof(int)));
        writeFileFromBuffer(
            path,
            std::vector<unsigned char>(
                reinterpret_cast<unsigned char*>(&size),
                reinterpret_cast<unsigned char*>(&size) + sizeof(int)),
            true);

        for (const auto& [key, value] : codeMap) {
            writeFileFromBuffer(path, std::vector<unsigned char>{key}, true);
            int len = value.size();
            writeFileFromBuffer(
                path,
                std::vector<unsigned char>(
                    reinterpret_cast<unsigned char*>(&len),
                    reinterpret_cast<unsigned char*>(&len) + sizeof(int)),
                true);
            writeFileFromBuffer(
                path, std::vector<unsigned char>(value.begin(), value.end()),
                true);
        }
    }

    void writeCodeToFile() {
        std::string codeContent;
        for (const auto& [key, value] : codeMap) {
            codeContent += std::string(1, key) + ":" + value + "\n";
        }

        std::ofstream out1(FOR_DECOMPRESSED_FILE.data());
        std::ofstream out(CODE_FILE.data());

        if (!out1 || !out) {
            throw std::runtime_error("Failed to open code files for writing");
        }

        out << writeToFile;
        out1 << codeContent;
    }

    void compress(const std::string& outputString, int paddedBits) {
        int size = outputString.size();
        std::vector<unsigned char> outputBuffer =
            getBufferFromString(outputString, size);
        writeHeader(COMPRESSED_FILE.data(), paddedBits);
        writeFileFromBuffer(COMPRESSED_FILE.data(), outputBuffer, true);
    }

    void getBitString(TreeNode* root, const std::vector<unsigned char>& buffer,
                      int size, long long int& sum) {
        std::string ini;
        treeTraversal(root, ini, sum);
        writeCodeToFile();

        std::string outString;
        outString.reserve(size * 8);
        for (int i = 0; i < size; i++) {
            outString += codeMap[buffer[i]];
        }

        int paddedBits = 0;
        if (outString.size() % 8 != 0) {
            int deficit = 8 * ((outString.size() / 8) + 1) - outString.size();
            paddedBits = deficit;
            outString.append(deficit, '0');
        }

        compress(outString, paddedBits);
    }

    TreeNode* createTree(const std::map<unsigned char, int>& frequencyMap) {
        std::priority_queue<TreeNode*, std::vector<TreeNode*>, Comp> pq;

        for (const auto& [key, freq] : frequencyMap) {
            pq.push(new TreeNode(key, freq));
        }

        while (pq.size() > 1) {
            TreeNode* left = pq.top();
            pq.pop();
            TreeNode* right = pq.top();
            pq.pop();

            TreeNode* newNode =
                new TreeNode('\0', right->frequency + left->frequency);
            newNode->left = left;
            newNode->right = right;
            pq.push(newNode);
        }

        return pq.top();
    }

   public:
    void compressFile() {
        try {
            std::ifstream file(INPUT_FILE.data(), std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open input file: " +
                                         std::string(INPUT_FILE.data()));
            }

            std::vector<unsigned char> buffer(
                std::istreambuf_iterator<char>(file), {});
            std::map<unsigned char, int> frequencyMap;

            for (unsigned char byte : buffer) {
                frequencyMap[byte]++;
            }

            long long int sum = 0;
            for (const auto& [_, freq] : frequencyMap) {
                sum += freq;
            }

            TreeNode* root = createTree(frequencyMap);
            getBitString(root, buffer, buffer.size(), sum);

            // Cleanup
            std::function<void(TreeNode*)> deleteTree =
                [&deleteTree](TreeNode* node) {
                    if (node) {
                        deleteTree(node->left);
                        deleteTree(node->right);
                        delete node;
                    }
                };
            deleteTree(root);

            Logger::info("File compression completed successfully");
        } catch (const std::exception& e) {
            Logger::error("Compression error: %s", e.what());
            throw;
        }
    }
};

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

// File sender class
class FileSender {
   private:
    static constexpr size_t CHUNK_SIZE = BUFFER_SIZE;

    void sendFileChunk(int socket, const std::vector<char>& chunk) {
        ssize_t totalSent = 0;
        while (totalSent < static_cast<ssize_t>(chunk.size())) {
            ssize_t sent = send(socket, chunk.data() + totalSent,
                                chunk.size() - totalSent, 0);
            if (sent < 0) {
                throw std::runtime_error("Failed to send data");
            }
            totalSent += sent;
        }
    }

   public:
    void sendFile(int socket, const std::string& filePath) {
        try {
            std::ifstream file(filePath, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Failed to open file: " + filePath);
            }

            std::vector<char> buffer(CHUNK_SIZE);
            size_t totalBytes = 0;

            while (file) {
                file.read(buffer.data(), buffer.size());
                std::streamsize bytesRead = file.gcount();
                if (bytesRead > 0) {
                    buffer.resize(bytesRead);
                    sendFileChunk(socket, buffer);
                    totalBytes += bytesRead;
                }
            }

            Logger::info("Sent %zu bytes from file: %s", totalBytes,
                         filePath.c_str());
        } catch (const std::exception& e) {
            Logger::error("Error sending file: %s", e.what());
            throw;
        }
    }
};

// Client class
class Client : public NetworkBase {
   private:
    std::unique_ptr<FileSender> fileSender;
    std::unique_ptr<HuffmanCompressor> compressor;
    std::vector<std::string> filesToSend;

    void connect() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        setSocketOptions();

        if (::connect(sockfd, reinterpret_cast<struct sockaddr*>(&addr),
                      sizeof(addr)) < 0) {
            throw std::runtime_error("Failed to connect to server");
        }

        Logger::info("Connected to server at %s:%d", ip.c_str(), port);
    }

   public:
    Client() : NetworkBase(SERVER_IP.data(), SERVER_PORT) {
        fileSender = std::make_unique<FileSender>();
        compressor = std::make_unique<HuffmanCompressor>();
        filesToSend = {COMPRESSED_FILE.data(), CODE_FILE.data(),
                       FOR_DECOMPRESSED_FILE.data(), COMPRESSED_FILE.data()};
    }

    void start() {
        try {
            // First compress the file
            compressor->compressFile();

            // Then send the files
            for (const auto& file : filesToSend) {
                connect();  // Create new connection for each file

                try {
                    fileSender->sendFile(sockfd, file);
                } catch (const std::exception& e) {
                    Logger::error("Failed to send file %s: %s", file.c_str(),
                                  e.what());
                }

                close(sockfd);
                sockfd = -1;
            }

            Logger::info("All files sent successfully");
        } catch (const std::exception& e) {
            Logger::error("Client error: %s", e.what());
            throw;
        }
    }
};

int main() {
    try {
        Logger::setLogFileName("client.log");
        Client client;
        client.start();
    } catch (const std::exception& e) {
        Logger::critical("Fatal error: %s", e.what());
        return 1;
    }
    Logger::close();
    return 0;
}
