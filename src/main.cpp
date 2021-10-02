#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <iterator>
#include <sstream>
#include <typeinfo>

using namespace std;

const int BLOCK_SIZE = 100;
const int NODE_N = 11;
const int TCONST_FIXED_LENGTH = 10;

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
        float getAverageRating() {
            return averageRating;
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

        int getBlockId() {
            return blockId;
        }

        void printContent() {
            cout << "Accessed block id: " << getBlockId() << ": ";
            cout << getRecords()[0].getTconst();
            // for (int i=0; i<getRecords().size(); i++) {
            //     cout << getRecords()[i].getTconst() << " ";
            // }
            cout << "\n";
        }

};


class Node {
    public:
        int* keyArr;
        bool isLeaf;
        int nodeSize;

        Node() {
            keyArr = new int[NODE_N];
            nodeSize = 0;
        };

        void printNode() {
            for (int i=0; i<nodeSize; i++) {
                cout << keyArr[i] << " ";
            }
            cout << "\n";
        }
};

class NonLeafNode: public Node {
    public:
        Node** nodePointerArr;
        NonLeafNode* parent;

        NonLeafNode(): Node() {
            nodePointerArr = new Node*[NODE_N+1];
            isLeaf = false;
            parent = NULL;
        }

};

class LeafNode: public Node {
    public: 
        vector<Block*> blockPointerArr[NODE_N];
        LeafNode* siblingNodePtr;
        NonLeafNode* parent;

        LeafNode(): Node() {
            isLeaf = true;
            parent = NULL;
        }


        void split() {
            return;
        }

};

class BPlusTree {
    public:
        Node* rootNode;

        BPlusTree() {
            rootNode = NULL;
        }

        void printRootNode() {
            rootNode->printNode();
        }

        pair<LeafNode*, NonLeafNode*> locateTargetLeafNode(int valueToInsert) {
            LeafNode* target;
            NonLeafNode* parent;
            if (rootNode == NULL) {
                target = new LeafNode();
                rootNode = target;
                parent = NULL;
            } else {
                Node* current = rootNode;
                while (!current->isLeaf) {
                    parent = static_cast<NonLeafNode*>(current);
                    for (int i=0; i<parent->nodeSize; i++) {
                        if (valueToInsert < parent->keyArr[i]) {
                            current = parent->nodePointerArr[i];
                            break;
                        }
                        if (i == current->nodeSize-1) {
                            // value is greater or equal to largest key in node, so we follow the last pointer
                            current = parent->nodePointerArr[parent->nodeSize];
                        }                        
                    }    
                }
                // current node is a leaf node
                target = static_cast<LeafNode*>(current);
            }
            return make_pair(target, parent);
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

                cout << "New parent node: ";
                target->printNode();
            } else {
                // parent node is full, need to split
                cout << "Splitting parent node with key: " << key << " and new child node containing numVotes: " << newChildPtr->keyArr[0] << endl;
                cout << "original parent node: ";
                target->printNode();

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

                cout << "final nodes: " ;
                target->printNode();
                newNonLeafNodePtr->printNode();

                int keyToInsert = bufferKeyArr[target->nodeSize];
                if (target->parent == NULL) {
                    NonLeafNode* newParent = new NonLeafNode();
                    newParent->keyArr[0] = keyToInsert;
                    newParent->nodePointerArr[0] = target;
                    newParent->nodePointerArr[1] = newNonLeafNodePtr;
                    newParent->nodeSize++;
                    target->parent = newParent;
                    newNonLeafNodePtr->parent = newParent;
                    rootNode = newParent;

                    cout << "New parent node: ";
                    newParent->printNode();

                } else {
                    insertIntoParentNode(target->parent, keyToInsert, newNonLeafNodePtr);
                }
            }
        }


        void insertIntoLeafNode(LeafNode* target, NonLeafNode* parent, int value, Block* newBlockPtr) {
            if (target->nodeSize == 0) {
                target->keyArr[0] = value;
                target->blockPointerArr[0].push_back(newBlockPtr);
                target->nodeSize++;
                return;
            } else if (target->nodeSize < NODE_N) {
                // finding index to insert new key
                int i = 0;
                while (target->keyArr[i] < value && i < target->nodeSize) {
                    i++;
                }
                if (target->keyArr[i] == value) {
                    target->blockPointerArr[i].push_back(newBlockPtr);
                } else {
                    // shifting larger keys and pointers right
                    for (int j=target->nodeSize; j>i; j--) {
                        target->keyArr[j] = target->keyArr[j-1];
                        target->blockPointerArr[j] = target->blockPointerArr[j-1];
                    }
                    target->keyArr[i] = value;
                    target->blockPointerArr[i].clear();
                    target->blockPointerArr[i].push_back(newBlockPtr);
                    target->nodeSize++;
                }
                return;
            } else {
                // node is full, searching if key exists
                int i = 0;
                while (target->keyArr[i] < value && i < target->nodeSize) {
                    i++;
                }
                if (target->keyArr[i] == value) {
                    target->blockPointerArr[i].push_back(newBlockPtr);
                } else {
                    // node is full, need to split
                    cout << "Splitting leaf node when adding key: " << value << endl;
                    cout << "original leaf node: ";
                    target->printNode();

                    LeafNode* newLeafNodePtr = new LeafNode();
                    newLeafNodePtr->parent = target->parent;
                    int bufferKeyArr[NODE_N+1];
                    vector<Block*> bufferBlockPtrArr[NODE_N+1];
                    for (int i = 0; i < NODE_N; i++) {
                        bufferKeyArr[i] = target->keyArr[i];
                        bufferBlockPtrArr[i] = target->blockPointerArr[i];
                    }

                    
                    int i = 0;
                    while (bufferKeyArr[i] < value && i < NODE_N) {
                        i++;
                    }
                    for (int j = NODE_N; j>i; j--) {
                        bufferKeyArr[j] = bufferKeyArr[j-1];
                        bufferBlockPtrArr[j] = bufferBlockPtrArr[j-1];
                    }
                    bufferKeyArr[i] = value;
                    bufferBlockPtrArr[i].clear();
                    bufferBlockPtrArr[i].push_back(newBlockPtr);
                    // new leaf node size = floor((n+1)/2)
                    newLeafNodePtr->nodeSize = (NODE_N + 1)/2;
                    // original leaf node size = ceiling((n+1)/2)
                    target->nodeSize = NODE_N + 1 - (NODE_N + 1)/2;
                    // allocate respective keys and pointers to the two nodes
                    newLeafNodePtr->siblingNodePtr = target->siblingNodePtr;
                    target->siblingNodePtr = newLeafNodePtr;
                    for (int i=0; i < target->nodeSize; i++) {
                        target->keyArr[i] = bufferKeyArr[i];
                        target->blockPointerArr[i] = bufferBlockPtrArr[i];
                    }
                    for (int i=0, j = target->nodeSize; i < newLeafNodePtr->nodeSize; i++, j++) {
                        newLeafNodePtr->keyArr[i] = bufferKeyArr[j];
                        newLeafNodePtr->blockPointerArr[i] = bufferBlockPtrArr[j];
                    }

                    cout << "final leaf nodes: ";
                    target->printNode();
                    newLeafNodePtr->printNode();

                    if (target->parent == NULL) {
                        parent = new NonLeafNode();
                        parent->keyArr[0] = newLeafNodePtr->keyArr[0];
                        parent->nodePointerArr[0] = target;
                        parent->nodePointerArr[1] = newLeafNodePtr;
                        parent->nodeSize++;
                        target->parent = parent;
                        newLeafNodePtr->parent = parent;
                        rootNode = parent;

                        cout << "New parent created: ";
                        parent->printNode();
                    } else {
                        insertIntoParentNode(parent, newLeafNodePtr->keyArr[0], newLeafNodePtr);
                    }

                }
            }
        }
        
        void insertIntoTree(int value, Block* newBlockPtr) {
            pair<LeafNode*, NonLeafNode*> targetLeafParentPair = locateTargetLeafNode(value);
            LeafNode* targetLeafNode = targetLeafParentPair.first;
            NonLeafNode* parentOfTarget = targetLeafParentPair.second;
            insertIntoLeafNode(targetLeafNode, parentOfTarget, value, newBlockPtr);
        }

        void searchValue(int value) {  
            LeafNode* target;
            NonLeafNode* parent;
            vector<Node*> accessedNodes;
            vector<Block*> accessedBlocks;
            bool found = false;
            if (rootNode == NULL) {
                cout << "target key " << value << " not found." << endl;
                return;
            } else {
                Node* current = rootNode;
                while (!current->isLeaf) {
                    parent = static_cast<NonLeafNode*>(current);
                    accessedNodes.push_back(parent);
                    for (int i=0; i<parent->nodeSize; i++) {
                        if (value < parent->keyArr[i]) {
                            current = parent->nodePointerArr[i];
                            break;
                        }
                        if (value == parent->keyArr[i]) {
                            current = parent->nodePointerArr[i+1];
                            break;
                        }
                        if (i == current->nodeSize-1) {
                            // value is greater or equal to largest key in node, so we follow the last pointer
                            current = parent->nodePointerArr[parent->nodeSize];
                        }                        
                    }    
                }
                // current node is a leaf node
                target = static_cast<LeafNode*>(current);
                accessedNodes.push_back(target);
                for (int i = 0; i < target->nodeSize; i++) {
                    if (target->keyArr[i] == value) {
                        // cout << "target key " <<  value << " found! adding block: " << target->blockPointerArr[i]->getBlockId() << " to list of accessed blocks." << endl;
                        accessedBlocks = target->blockPointerArr[i];
                        found = true;
                        // searching subsequent keys to see if they match the search value
                        // do {
                        //     if (i == target->nodeSize-1) {
                        //         target = target->siblingNodePtr;
                        //         accessedNodes.push_back(target);
                        //         i = 0;
                        //     } else {
                        //         i++;
                        //         if (target->keyArr[i] == value) {
                        //             cout << "target key " <<  value << " found! adding block: " << target->blockPointerArr[i]->getBlockId() << " to list of accessed blocks." << endl;
                        //             accessedBlocks.push_back(target->blockPointerArr[i]);
                        //         }
                        //     }
                        // } while (target->keyArr[i] == value);
                        break;
                    }
                }
                if (found) {
                    // printing accessed nodes
                    cout << "Number of index nodes accessed: " << accessedNodes.size() << endl;
                    cout << "Content of up to first 5 index nodes: " << endl;
                    int counter = 0;
                    while (counter < accessedNodes.size() && counter < 5) {
                        cout << counter+1 << ": ";
                        accessedNodes[counter]->printNode();
                        cout << "\n";
                        counter++;
                }

                    // printing accessed blocks
                    cout << "Number of data blocks accessed: " << accessedBlocks.size() << endl;
                    cout << "Content of up to first 5 data blocks: " << endl;
                    accessedBlocks[0]->printContent();
                    // for (int i=0; i<accessedBlocks.size(); i++) {
                    //     cout << i+1 << ": ";
                    //     accessedBlocks[i]->printContent();
                    //     cout << "\n";
                    //     if (i==4) {
                    //         break;
                    //     }
                    // }
                    
                    counter = 0;
                    while (counter < accessedBlocks.size() && counter < 5) {
                        cout << counter+1 << ": ";
                        accessedBlocks.at(counter)->printContent();
                        cout << "\n";
                        counter++;
                    }

                    // getting average of average ratings from records retrieved
                    float sumOfAverageRatings = 0;
                    int recordCount = 0;
                    for (int i=0; i<accessedBlocks.size(); i++) {
                        for (int j=0; i<accessedBlocks[i]->getRecords().size(); j++) {
                            if (accessedBlocks[i]->getRecords()[j].getNumVotes() == value) {
                                sumOfAverageRatings += accessedBlocks[i]->getRecords()[i].getAverageRating();
                                recordCount++;
                            }
                        }
                    }
                    float averageOfAverageRatings = sumOfAverageRatings/recordCount;
                    cout << "Average of averageRatings for movies with numVotes: " << value << " is " << averageOfAverageRatings << endl;
                }      
            }     
        }
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
	// ifstream data ("./data.tsv");
    ifstream data ("C:\\Users\\Randy\\Desktop\\CZ4031 DBSP\\Project 1\\code\\src\\datatest.tsv");
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
        } catch (...) {
            continue;
        }

	}
    cout << "Storage of data complete!" << endl;
    cout << "Number of blocks: " << storage.size() << endl;
    float databaseSize = float(storage.size() * BLOCK_SIZE) / float(1000000);
    cout << "Size of database: " << databaseSize << "MB" << endl;
    cout << "Beginning to build B+ tree on numVotes.." << endl;
    BPlusTree* bPlusTree = new BPlusTree();
    for (int i=0; i<storage.size(); i++) {
        Block blockToIndex = storage[i];
        for (int j=0; j<blockToIndex.getRecords().size(); j++) {
            Record recordToIndex = storage[i].getRecords()[j];
            // cout << "inserting block id: " << blockToIndex.getBlockId() << " with index on numVotes: " << recordToIndex.getNumVotes() << " from record: " << recordToIndex.getTconst() << endl;
            if (recordToIndex.getNumVotes() > 6000000) {
                cout << recordToIndex.getTconst() << " CHECK " << recordToIndex.getNumVotes() << endl;
            } 
            bPlusTree->insertIntoTree(recordToIndex.getNumVotes(), &blockToIndex);
        }
    }
    int searchValue = 500;
    bPlusTree->searchValue(searchValue);
    bPlusTree->printRootNode();

    delete newBlockPtr;
}




