#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <bitset>

using namespace std;

// Huffman Tree Node
struct Node {
    unsigned char ch;
    int freq;
    Node* left;
    Node* right;

    Node(unsigned char character, int frequency) : ch(character), freq(frequency), left(nullptr), right(nullptr) {}
};

// Comparison object for priority queue
struct compare {
    bool operator()(Node* left, Node* right) {
        return left->freq > right->freq;
    }
};

// Function to traverse the Huffman Tree and store Huffman Codes
void encode(Node* root, string str, unordered_map<unsigned char, string>& huffmanCode) {
    if (!root)
        return;

    if (!root->left && !root->right) {
        huffmanCode[root->ch] = str.empty() ? "0" : str; // Handle single character case
    }

    encode(root->left, str + "0", huffmanCode);
    encode(root->right, str + "1", huffmanCode);
}

// Function to build the Huffman Tree
Node* buildHuffmanTree(const unordered_map<unsigned char, int>& freqMap) {
    priority_queue<Node*, vector<Node*>, compare> pq;

    for (auto pair : freqMap) {
        pq.push(new Node(pair.first, pair.second));
    }

    if (pq.size() == 1) {
        pq.push(new Node(0, 1));
    }

    while (pq.size() != 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();

        int sum = left->freq + right->freq;
        Node* newNode = new Node(0, sum);
        newNode->left = left;
        newNode->right = right;

        pq.push(newNode);
    }

    return pq.top();
}

// Serialize Huffman Tree (pre-order traversal)
void serializeTree(Node* root, ostream& out) {
    if (!root)
        return;

    if (!root->left && !root->right) {
        out.put('1');
        out.put(root->ch);
    } else {
        out.put('0');
        serializeTree(root->left, out);
        serializeTree(root->right, out);
    }
}

// Deserialize Huffman Tree
Node* deserializeTree(ifstream& in) {
    char flag;
    in.get(flag);
    if (flag == '1') {
        char byte;
        in.get(byte);
        return new Node(static_cast<unsigned char>(byte), 0);
    } else {
        Node* left = deserializeTree(in);
        Node* right = deserializeTree(in);
        Node* parent = new Node(0, 0);
        parent->left = left;
        parent->right = right;
        return parent;
    }
}

// Generate frequency map from input file
unordered_map<unsigned char, int> generateFrequencyMap(const string& fileData) {
    unordered_map<unsigned char, int> freqMap;
    for (unsigned char ch : fileData) {
        freqMap[ch]++;
    }
    return freqMap;
}

// Convert bit string to byte vector
vector<unsigned char> bitStringToBytes(const string& bitString) {
    vector<unsigned char> bytes;
    unsigned char byte = 0;
    int count = 0;

    for (char bit : bitString) {
        byte = (byte << 1) | (bit - '0');
        count++;
        if (count == 8) {
            bytes.push_back(byte);
            byte = 0;
            count = 0;
        }
    }

    if (count > 0) {
        byte = byte << (8 - count);
        bytes.push_back(byte);
    }

    return bytes;
}

// Convert byte vector to bit string
string bytesToBitString(const vector<unsigned char>& bytes, int extra_bits) {
    string bitString = "";
    for (size_t i = 0; i < bytes.size(); ++i) {
        bitString += bitset<8>(bytes[i]).to_string();
    }
    if (extra_bits > 0) {
        bitString.erase(bitString.size() - (8 - extra_bits), 8 - extra_bits);
    }
    return bitString;
}

// Compress file
void compressFile(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Error opening input file!" << endl;
        return;
    }

    string fileData((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    unordered_map<unsigned char, int> freqMap = generateFrequencyMap(fileData);
    Node* root = buildHuffmanTree(freqMap);

    unordered_map<unsigned char, string> huffmanCode;
    encode(root, "", huffmanCode);

    string encodedString = "";
    for (unsigned char ch : fileData) {
        encodedString += huffmanCode[ch];
    }

    int extra_bits = 8 - (encodedString.size() % 8);
    if (extra_bits != 8) {
        encodedString.append(extra_bits, '0');
    } else {
        extra_bits = 0;
    }

    vector<unsigned char> encodedBytes = bitStringToBytes(encodedString);

    ofstream out(outputFile, ios::binary);
    if (!out) {
        cerr << "Error opening output file!" << endl;
        return;
    }

    out.write(reinterpret_cast<char*>(&extra_bits), sizeof(extra_bits));

    serializeTree(root, out);

    out.write(reinterpret_cast<char*>(encodedBytes.data()), encodedBytes.size());
    out.close();

    cout << "File compressed successfully!" << endl;
}

// Decompress file
void decompressFile(const string& inputFile, const string& outputFile) {
    ifstream in(inputFile, ios::binary);
    if (!in) {
        cerr << "Error opening compressed file!" << endl;
        return;
    }

    int extra_bits = 0;
    in.read(reinterpret_cast<char*>(&extra_bits), sizeof(extra_bits));

    Node* root = deserializeTree(in);

    vector<unsigned char> encodedBytes((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    string bitString = bytesToBitString(encodedBytes, extra_bits);

    string decodedData = "";
    Node* current = root;
    for (char bit : bitString) {
        if (bit == '0')
            current = current->left;
        else
            current = current->right;

        if (!current->left && !current->right) {
            decodedData += current->ch;
            current = root;
        }
    }

    ofstream out(outputFile, ios::binary);
    if (!out) {
        cerr << "Error opening output file!" << endl;
        return;
    }

    out.write(decodedData.c_str(), decodedData.size());
    out.close();

    cout << "File decompressed successfully!" << endl;
}

// Display usage instructions
void printUsage(const string& programName) {
    cout << "Usage:\n";
    cout << "  To compress:   " << programName << " -c <input_file> <output_file>\n";
    cout << "  To decompress: " << programName << " -d <input_file> <output_file>\n";
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    string mode = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];

    if (mode == "-c") {
        compressFile(inputFile, outputFile);
    } else if (mode == "-d") {
        decompressFile(inputFile, outputFile);
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
