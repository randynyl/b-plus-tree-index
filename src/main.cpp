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
            this->recordLength = tconst.size() + sizeof(averageRating) + sizeof(numVotes) + sizeof(int);
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
        // blockId as part of block header
        int blockId;
        int recordLength;
        // records within a block are simulated to be physically contiguous.
        // records need not be sequenced in order since we're using a b+ tree index.
        vector<Record> records;
	public:
		Block() {}
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
        int* keyArr;
        Node() {
            keyArr = new int[NODE_N];
        }
};

class LeafNode: public Node {
    public: 
        Node* siblingNodePtr;
        Record** recordPointerArr;

        LeafNode(): Node() {
            recordPointerArr = new Record*[NODE_N];
        }

        // void insertIntoLeaf(int value, Record* recordPtr) {}
        // void split() {}

};

class NonLeafNode: public Node {
    public:
        Node** nodePointerArr;

        NonLeafNode(): Node() {
            nodePointerArr = new Node*[NODE_N+1];
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
        
        void insertIntoTree(int value, Record* recordPtr) {
            if (rootNode == NULL) {
                LeafNode* firstNode = new LeafNode();
                firstNode->keyArr[0] = value;
                firstNode->recordPointerArr[0] = recordPtr;
                rootNode = firstNode;

                // need to successfully check that our root node is currently a leafNode.
                // currently, this prints 0 but it should print 1
                cout << (typeid(*rootNode) == typeid(LeafNode)) << endl;
            } else {
                Node* current = rootNode;

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
    Block* newBlockPtr = new Block();
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
                newBlockPtr = new Block();
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



