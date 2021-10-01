#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <sstream>
#include <typeinfo>

using namespace std;

int BLOCK_SIZE = 100;
int NODE_N = 4;
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
        int* keyArr;
        bool isLeaf;
        int nodeSize;

        // attributes used by LeafNode
        // vector<Record*> recordPointerArr;
        // Node* siblingNodePtr;

        // attribute used my NonLeafNode
        // vector<Node*> nodePointerArr;

        Node() {
            keyArr = new int[NODE_N];
            nodeSize = 0;
        };
};

class LeafNode: public Node {
    public: 
        Record** recordPointerArr;
        Node* siblingNodePtr;

        LeafNode(): Node() {
            recordPointerArr = new Record*[NODE_N];
            isLeaf = true;
        }

        // void insertIntoLeaf(int value, Record* newRecordPtr) {}
        void split() {
            return;
        }

};

class NonLeafNode: public Node {
    public:
        Node** nodePointerArr;

        NonLeafNode(): Node() {
            nodePointerArr = new Node*[NODE_N+1];
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

        pair<LeafNode*, NonLeafNode*> locateTargetLeafNode(int valueToInsert) {
            LeafNode* target;
            NonLeafNode* parent;
             
            if (rootNode == NULL) {
                target = new LeafNode();
                rootNode = target;
           
                //parent = NULL;
            } else {
                Node* current = rootNode;
                NonLeafNode* parent;
                current->isLeaf = true;
                
                while (!current->isLeaf) {
                    parent = static_cast<NonLeafNode*>(current);
                    for (int i=0; i<current->nodeSize; i++) {
                        if (valueToInsert < current->keyArr[i]) {
                            current = parent->nodePointerArr[i];
                            break;
                        }                        
                    }
                    // value is greater or equal to largest key in node, so we follow the last pointer
                    current = parent->nodePointerArr[current->nodeSize];
                }
                // current node is a leaf node
                target = static_cast<LeafNode*>(current);
                //insertIntoLeafNode(LeafNode,NonLeafNode,target,Record);


            }
            return make_pair(target, parent);
        }

        NonLeafNode* findParentNode(Node* target, Node* currentNode) {
            NonLeafNode* parentNode;
            if (currentNode->isLeaf) {
                return NULL;
            }
            NonLeafNode* current = static_cast<NonLeafNode*>(currentNode);
            // Recursive depth first search for child
            for (int i=0; i<current->nodeSize+1; i++) {
                if (current->nodePointerArr[i] == target) {
                    parentNode = current;
                    return parentNode;
                } else {
                    parentNode = findParentNode(target, current->nodePointerArr[i]);
                }
            }
            return NULL;
        }
        
        void insertIntoParentNode(NonLeafNode* target, int key, Node* newChildPtr) {
            if (target->nodeSize < NODE_N) {
                int i = 0;
                // finding index to insert new key
                while (target->keyArr[i] < key && i < target->nodeSize) {
                    i++;
                }
                for (int j=target->nodeSize; j>i; j--) {
                    target->keyArr[j] = target->keyArr[j-1];
                    target->nodePointerArr[j+1] = target->nodePointerArr[j];
                }
                target->keyArr[i] = key;
                target->nodePointerArr[i+1] = newChildPtr;
                target->nodeSize++;
            } else {
                // parent node is full, need to split
                cout << "Splitting parent node with key: " << key << " and new child node containing numVotes: " << newChildPtr->keyArr[0] << endl;
                NonLeafNode* newNonLeafNodePtr = new NonLeafNode();
                int bufferKeyArr[NODE_N+1];
                Node* bufferNodePtrArr[NODE_N+2];
                // add keys and pointer to a buffer in sequence
                for (int i=0; i<target->nodeSize; i++) {
                    bufferKeyArr[i] = target->keyArr[i];
                    bufferNodePtrArr[i] = target->nodePointerArr[i];
                }
                bufferNodePtrArr[NODE_N] = target->nodePointerArr[NODE_N];

                int i = 0;
                while (target->keyArr[i] < key && i < target->nodeSize) {
                    i++;
                }
                for (int j=target->nodeSize+1; j>i; j--) {
                    bufferKeyArr[j] = bufferKeyArr[j-1];
                    bufferNodePtrArr[j+1] = bufferNodePtrArr[j];
                }
                bufferKeyArr[i] = key;
                bufferNodePtrArr[i+1] = newChildPtr;
                // new node contains at least floor(n/2) keys
                newNonLeafNodePtr->nodeSize = NODE_N/2;
                target->nodeSize = NODE_N - NODE_N/2;
                for (int i=0, j = target->nodeSize + 1; i<newNonLeafNodePtr->nodeSize; i++, j++) {
                    newNonLeafNodePtr->keyArr[i] = bufferKeyArr[j];
                }
                for (int i=0, j = target->nodeSize + 1; i < newNonLeafNodePtr->nodeSize + 1; i++, j++) {
                    newNonLeafNodePtr->nodePointerArr[i] = bufferNodePtrArr[j]; 
                }
                int keyToInsert = bufferKeyArr[target->nodeSize];
                if (target == rootNode) {
                    NonLeafNode* newParent = new NonLeafNode();
                    newParent->keyArr[0] = keyToInsert;
                    newParent->nodePointerArr[0] = target;
                    newParent->nodePointerArr[1] = newNonLeafNodePtr;
                    newParent->nodeSize++;
                    rootNode = newParent;
                } else {
                    insertIntoParentNode(findParentNode(rootNode, target), keyToInsert, newNonLeafNodePtr);
                }

            }

        }


        void insertIntoLeafNode(LeafNode* target, NonLeafNode* parent, int value, Record* newRecordPtr) {
            if (target->nodeSize == 0) {
                target->keyArr[0] = value;
                target->recordPointerArr[0] = newRecordPtr;
                target->nodeSize++;
                return;
            } else if (target->nodeSize < NODE_N) {
                // finding index to insert new key
                int i = 0;
                while (target->keyArr[i] < value && i < target->nodeSize) {
                    i++;
                }
                // shifting larger keys and pointers right
                for (int j=target->nodeSize; j>i; j--) {
                    target->keyArr[j] = target->keyArr[j-1];
                    target->recordPointerArr[j] = target->recordPointerArr[j-1];
                }
                target->keyArr[i] = value;
                target->recordPointerArr[i] = newRecordPtr;
                target->nodeSize++;
                return;
            } else {
                // node is full, need to split
               // cout << "Splitting leaf node when adding key: " << value << " that contains numVotes: " << target->keyArr[0] << endl;
                LeafNode* newLeafNodePtr = new LeafNode();
                int bufferKeyArr[NODE_N+1];
                Record* bufferRecordPtrArr[NODE_N+1];
                for (int i = 0; i < NODE_N; i++) {
                    bufferKeyArr[i] = target->keyArr[i];
                    bufferRecordPtrArr[i] = target->recordPointerArr[i];
                }
                int i = 0;
                while (bufferKeyArr[i] < value && i < NODE_N) {
                    i++;
                }
                for (int j = NODE_N+1; j>i; j--) {
                    bufferKeyArr[j] = bufferKeyArr[j-1];
                    bufferRecordPtrArr[j] = bufferRecordPtrArr[j-1];
                }
                bufferKeyArr[i] = value;
                bufferRecordPtrArr[i] = newRecordPtr;
                // new leaf node size = floor((n+1)/2)
                newLeafNodePtr->nodeSize = (NODE_N + 1)/2;
                // original leaf node size = ceiling((n+1)/2)
                target->nodeSize = NODE_N + 1 - (NODE_N + 1)/2;
                // allocate respective keys and pointers to the two nodes
                newLeafNodePtr->siblingNodePtr = target->siblingNodePtr;
                target->siblingNodePtr = newLeafNodePtr;
                for (int i=0; i < target->nodeSize; i++) {
                    target->keyArr[i] = bufferKeyArr[i];
                    target->recordPointerArr[i] = bufferRecordPtrArr[i];
                }
                for (int i=0, j = target->nodeSize; i < newLeafNodePtr->nodeSize; i++, j++) {
                    newLeafNodePtr->keyArr[i] = bufferKeyArr[j];
                    newLeafNodePtr->recordPointerArr[i] = bufferRecordPtrArr[j];
                }
                if (target == rootNode) {
                    parent = new NonLeafNode();
                    parent->keyArr[0] = newLeafNodePtr->keyArr[0];
                    parent->nodePointerArr[0] = target;
                    parent->nodePointerArr[1] = newLeafNodePtr;
                    parent->nodeSize++;
                    rootNode = parent;
                }
                insertIntoParentNode(parent, newLeafNodePtr->keyArr[0], newLeafNodePtr);
            }
        }
        
        void insertIntoTree(int value, Record* newRecordPtr) {
           // cout << "inserting id: " << newRecordPtr->getTconst() << " with index on numVotes: " << value << endl;
            pair<LeafNode*, NonLeafNode*> targetLeafParentPair = locateTargetLeafNode(value);
            LeafNode* targetLeafNode = targetLeafParentPair.first;
            NonLeafNode* parentOfTarget = targetLeafParentPair.second;
            insertIntoLeafNode(targetLeafNode, parentOfTarget, value, newRecordPtr);
            
        }

        void searchValue(int searchvalue1) {
        
        
        if (rootNode == NULL) {
            cout << "Tree is empty\n";
        } else {
        Node *current = rootNode;
        while (!current->isLeaf) {
        for (int i = 0; i < current->nodeSize; i++) {
        if (searchvalue1 < current->keyArr[i]) {
          current = current;
          break;
        }
        if (i == current->nodeSize - 1) {
          current = current+1;
          break;
        }
      }
    }
    for (int i = 0; i < current->nodeSize; i++) {
      if (current->keyArr[i] == searchvalue1) {
        cout << "Found\n";
        return;
      }
    }
    cout << "Not found\n";
    }
 }
           
         \

        // void deleteFromTree(int value) {}
        // vector<Record> searchValue(int value) {}
        // vector<Record> searchRangeOfValues(int lowerBound, int upperBound) {}
        // Node getParent(Node* nodePtr) {}

};
        

int main(int argc, char *argv[])
{
	cout << "Program begins: " << endl;
    vector<Block> storage;
    int blockId = 0;
    Block* newBlockPtr = new Block(blockId);
    storage.push_back(*newBlockPtr);
    // begin reading tsv file and storing into blocks
    cout << "Reading data and storing into blocks.." << endl;
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
    cout << "Storage of data complete!" << endl;
    cout << "Number of blocks: " << storage.size() << endl;
    cout << "Size of database: " << storage.size() * BLOCK_SIZE / 1000000 <<  endl;
    delete newBlockPtr;
    cout << "Beginning to build B+ tree on numVotes.." << endl;
    BPlusTree* bPlusTree = new BPlusTree();
    for (int i=0; i<storage.size(); i++) {
        for (int j=0; j<storage[i].getRecords().size(); j++) {
            Record recordToIndex = storage[i].getRecords()[j];
            bPlusTree->insertIntoTree(recordToIndex.getNumVotes(), &recordToIndex);
        }
    }
        int test;
        cout << "Type a number: "; // Type a number and press enter
        cin >> test; // Get user input from the keyboard
        
          

        BPlusTree o;
        o.searchValue(test);
   
}
