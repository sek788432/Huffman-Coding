# Huffman-Coding
Using Huffman Coding algorithm to compress files in client and decompress files in server.

## Introduction
This is a repo of Huffman-Coding implementation.<br />
First, the client will compress a file and generate the code via Huffman-Coding.<br />
Then, the client will pass a compressed file and the code generate by Huffman-Coding to server.<br />
Last, the server will decompress the compressed file with the code passed by the client.

---

## Repo Structure
```bash
.
├── client_place
│   ├── client.cpp
│   ├── dog.jpeg
│   ├── Makefile
│   ├── song.mp3
│   └── test.jpg
├── README.md
└── server_place
    ├── Makefile
    └── server.cpp

```

---

## Execution
### Server
```sh
make
./server
make clean
```

### client
```sh
make
./client
make clean
```
