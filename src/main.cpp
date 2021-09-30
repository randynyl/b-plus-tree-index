#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <sstream>
#include <typeinfo>

using namespace std;

int BLOCK_SIZE = 100;
int NODE_N = 12;
int TCONST_FIXED_LENGTH = 10;
std::vector<std::string> split(const std::string &line, char delimiter) {
    std::stringstream ss;
    ss.str(line);
    std::string item;
	vector<string> elements;
    while (std::getline(ss, item, delimiter)) {
        elements.push_back(item);
    }
	return elements;
}

class Record {
	private:
        // record length as part of record header
        int recordLength;
        string tconst;
        float averageRating;
        int numVotes;
	public:
		Record(string tconst, float averageRating, int numVotes) {
			this->tconst = tconst;
			this->averageRating = averageRating;
			this->numVotes = numVotes;
            // size in bytes of tconst + averageRating + numVotes + recordLength
            this->recordLength = TCONST_FIXED_LENGTH + sizeof(averageRating) + sizeof(numVotes) + sizeof(int);
        }
        int getRecordLength() {
            return recordLength;
        }
        string getTconst() {
            return tconst;
        }
        int getNumVotes() {
            return numVotes;
        }
};

class Block {
	private:
        // blockId and recordLength as part of block header
        int blockId;
        int recordLength;
        // records within a block are simulated to be physically contiguous.
        // records need not be sequenced in order since we're using a b+ tree index.
        vector<Record> records;
	public:
		Block(int id) {
            blockId = id;
        }
		void* operator new(size_t size) {
            // Creating new block with allocated memory of size: BLOCK_SIZE
			void* p = ::operator new(BLOCK_SIZE);
			return p;
		}
		void addRecord(Record record) {
        if (records.size() == 0) {
            recordLength = record.getRecordLength();
        }
			records.push_back(record);
		}
        int getSize() {
            return sizeof(blockId) + sizeof(recordLength) + (recordLength * records.size());
        }
        vector<Record> getRecords() {
            return records;
        }

};


class Node {
    public:
        vector<int> keyArr;
        bool isLeaf;

        // attributes used by LeafNode
        vector<Record*> recordPointerArr;
        Node* siblingNodePtr;

        // attribute used my NonLeafNode
        vector<Node*> nodePointerArr;

        Node() = default;
        virtual void split() = 0;
};

class LeafNode: public Node {
    public: 
        LeafNode(): Node() {
            isLeaf = true;
        }

        // void insertIntoLeaf(int value, Record* newRecordPtr) {}
        void split() {
            return;
        }

};

class NonLeafNode: public Node {
    public:
        NonLeafNode(): Node() {
            isLeaf = false;
        }

        // void insertIntoNonLeaf(int value) {}
        // void split() {}
};

class BPlusTree {
    public:
        Node* rootNode;

        BPlusTree() {
            rootNode = NULL;
        }
        
        void insertIntoTree(int value, Record* newRecordPtr) {
            if (rootNode == NULL) {
                LeafNode* firstNode = new LeafNode();
                firstNode->keyArr.push_back(value);
                firstNode->recordPointerArr.push_back(newRecordPtr);
                rootNode = firstNode;
            } else {
                Node* current = rootNode;
                while (!current->isLeaf) {
                    for (int i=0; i<current->keyArr.size(); i++) {
                        if (value < current->keyArr[i]) {
                            current = current->nodePointerArr[i];
                            break;
                        }                        
                    }
                    // value is greater than largest key in node, so we follow the last pointer
                    current = current->nodePointerArr[current->keyArr.size()];
                }
                // current node is a leaf node
                int sizeKeys = current->keyArr.size();
                if (sizeKeys < NODE_N) {
                    // finding index to insert new key
                    int i = 0;
                    while (current->keyArr[i] < value && i < sizeKeys) {
                        i++;
                    }
                    // shifting larger keys and pointers right
                    for (int j=sizeKeys; j>i; j--) {
                        current->keyArr[j] = current->keyArr[j-1];
                        current->recordPointerArr[j] = current->recordPointerArr[j-1];
                    }
                    current->keyArr[i] = value;
                    current->recordPointerArr[i] = newRecordPtr;
                } else {
                    // node is full, need to split
                    LeafNode* newLeafNodePtr = new LeafNode();


                }
            }
        }
        // void deleteFromTree(int value) {}
        // vector<Record> searchValue(int value) {}
        // vector<Record> searchRangeOfValues(int lowerBound, int upperBound) {}
        // Node getParent(Node* nodePtr) {}

};


int main(int argc, char *argv[])
{
	cout << "Program begins: " << std::endl;
    vector<Block> storage;
    int blockId = 0;
    Block* newBlockPtr = new Block(blockId);
    storage.push_back(*newBlockPtr);
    // begin reading tsv file and storing into blocks
	ifstream data ("./data.tsv");
	string line;
	while (std::getline(data, line)) {
		vector<string> row_values;
		row_values = split(line, '\t');
        try {
            string tconst = row_values[0];
            float averageRating = stof(row_values[1]);
            int numVotes = stoi(row_values[2]);
            Record newRecord = Record(tconst, averageRating, numVotes);
            if (storage.back().getSize() + newRecord.getRecordLength() > BLOCK_SIZE) {
                blockId++;
                newBlockPtr = new Block(blockId);
                storage.push_back(*newBlockPtr);
            }
            storage.back().addRecord(newRecord);
            // cout << "Record: " << storage.back().getRecords().back().getTconst() << " added!" << endl;
        } catch (...) {
            continue;
        }

	}
    cout << "Number of blocks: " << storage.size() << endl;
    cout << "Size of database: " << storage.size() * BLOCK_SIZE << endl;
    cout << "Beginning to build B+ tree on numVotes.." << endl;
    BPlusTree* bPlusTree = new BPlusTree();
    Record firstRecord = storage[0].getRecords()[0];

    bPlusTree->insertIntoTree(firstRecord.getNumVotes(), &firstRecord);
	delete newBlockPtr;
}



