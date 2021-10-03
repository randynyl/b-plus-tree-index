#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <iterator>
#include <sstream>
#include <typeinfo>
#include <iomanip>

using namespace std;

const int BLOCK_SIZE = 100;
// each node has n int keys, n+1 pointers, 1 isLeaf boolean, 1 nodeSize int
// hence, n = floor([BLOCK_SIZE - size(int) - size(bool) - size(pointer)] / [size(int) + size(pointer)])
const int NODE_N = (BLOCK_SIZE - 9)/8;
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
            cout << "Accessed block id: " << getBlockId() << ", tconst values of records: ";
            int numRecords = getRecords().size();
            for (int i=0; i<getRecords().size(); i++) {
                cout << getRecords()[i].getTconst() << " ";
            }
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
        int numNodes;
        int height;

        BPlusTree() {
            rootNode = NULL;
            numNodes = 0;
            height = 0;
        }

        void printRootNode() {
            cout << "Root node: "; 
            rootNode->printNode();
        }

        void printRootFirstChild() {
            cout << "Root node's first child node: ";
            NonLeafNode* root = static_cast<NonLeafNode*>(rootNode);
            root->nodePointerArr[0]->printNode();
        }

        int getHeight() {
            return height;
        }

        int getNumNodes() {
            return numNodes;
        }

        pair<LeafNode*, NonLeafNode*> locateTargetLeafNode(int valueToInsert) {
            LeafNode* target;
            NonLeafNode* parent;
            if (rootNode == NULL) {
                target = new LeafNode();
                rootNode = target;
                parent = NULL;
                numNodes++;
                height++;
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

            } else {
                // parent node is full, need to split
                NonLeafNode* newNonLeafNodePtr = new NonLeafNode();
                numNodes++;
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
                for (int j=target->nodeSize; j>i; j--) {
                    bufferKeyArr[j] = bufferKeyArr[j-1];
                }

                for (int j=target->nodeSize+1; j>i+1; j--) {
                    bufferNodePtrArr[j] = bufferNodePtrArr[j-1];
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
                for (int i=0; i < target->nodeSize; i++) {
                    target->keyArr[i] = bufferKeyArr[i];
                }
                for (int i=0; i < target->nodeSize+1; i++) {
                    target->nodePointerArr[i] = bufferNodePtrArr[i];
                }
                newNonLeafNodePtr->parent = target->parent;


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
                    height++;
                    numNodes++;

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

                    LeafNode* newLeafNodePtr = new LeafNode();
                    numNodes++;
                    newLeafNodePtr->parent = target->parent;
                    int bufferKeyArr[NODE_N+1];
                    vector<vector<Block*>> bufferBlockPtrArr(NODE_N+1);
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

                    if (target->parent == NULL) {
                        parent = new NonLeafNode();
                        parent->keyArr[0] = newLeafNodePtr->keyArr[0];
                        parent->nodePointerArr[0] = target;
                        parent->nodePointerArr[1] = newLeafNodePtr;
                        parent->nodeSize++;
                        target->parent = parent;
                        newLeafNodePtr->parent = parent;
                        rootNode = parent;
                        numNodes++;
                        height++;
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
                        accessedBlocks = target->blockPointerArr[i];
                        found = true;
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
                        counter++;
                    }

                    // printing accessed blocks
                    int numBlocks = accessedBlocks.size();
                    accessedBlocks.resize(numBlocks + 1);
                    cout << endl << "Number of data blocks accessed: " << accessedBlocks.size() << endl;
                    cout << "Content of up to first 5 data blocks: " << endl;

                    for (int i=0; i<5; i++) {
                        cout << "Block " << i+1 << ": ";
                        vector<Record> records = accessedBlocks[i]->getRecords();
                        for (int j=0; j<records.size(); j++) {
                            cout << records[j].getTconst() << " ";
                        }
                        cout << "\n";
                        if (i == numBlocks-1) {
                            break;
                        }
                    }

                    // getting average of average ratings from records retrieved
                    float sumOfAverageRatings = 0;
                    int recordCount = 0;
                    for (int i=0; i<accessedBlocks.size()-1; i++) {
                        vector<Record> records = accessedBlocks[i]->getRecords();
                        for (int j=0; j<records.size(); j++) {
                            if (records[j].getNumVotes() == value) {
                                sumOfAverageRatings += records[j].getAverageRating();
                                recordCount++;
                            }
                        }
                    }
                    float averageOfAverageRatings = sumOfAverageRatings/recordCount;
                    cout << endl << "Average of averageRatings for movies with numVotes: " << value << " is " << setprecision(2) << averageOfAverageRatings << endl;
                }      
            }     
        }

        void searchValueRangeInclusive(int lowerBound, int upperBound) {  
            LeafNode* target;
            NonLeafNode* parent;
            vector<Node*> accessedNodes;
            vector<Block*> accessedBlocks;
            bool found = false;
            if (rootNode == NULL) {
                cout << "target keys not found." << endl;
                return;
            } else {
                Node* current = rootNode;
                while (!current->isLeaf) {
                    parent = static_cast<NonLeafNode*>(current);
                    accessedNodes.push_back(parent);
                    for (int i=0; i<parent->nodeSize; i++) {
                        if (lowerBound < parent->keyArr[i]) {
                            current = parent->nodePointerArr[i];
                            break;
                        }
                        if (lowerBound == parent->keyArr[i]) {
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
                    if (target->keyArr[i] >= lowerBound && target->keyArr[i] <= upperBound) {
                        accessedBlocks.insert(accessedBlocks.end(), target->blockPointerArr[i].begin(), target->blockPointerArr[i].end());
                        found = true;
                    }
                    // searching subsequent keys to see if they match the search value
                    do {
                        if (i == target->nodeSize-1) {
                            target = target->siblingNodePtr;
                            accessedNodes.push_back(target);
                            i = 0;
                        } else {
                            i++;
                            if (target->keyArr[i] >= lowerBound && target->keyArr[i] <= upperBound) {
                                accessedBlocks.insert(accessedBlocks.end(), target->blockPointerArr[i].begin(), target->blockPointerArr[i].end());
                                found = true;
                            }
                        }
                    } while (target->keyArr[i] <= upperBound);
                    break;
                }
                if (found) {
                    // printing accessed nodes
                    cout << "Number of index nodes accessed: " << accessedNodes.size() << endl;
                    cout << "Content of up to first 5 index nodes: " << endl;
                    int counter = 0;
                    while (counter < accessedNodes.size() && counter < 5) {
                        cout << counter+1 << ": ";
                        accessedNodes[counter]->printNode();
                        counter++;
                    }

                    // printing accessed blocks
                    int numBlocks = accessedBlocks.size();
                    cout << endl << "Number of data blocks accessed: " << numBlocks << endl;
                    cout << "Content of up to first 5 data blocks: " << endl;

                    for (int i=0; i<5; i++) {
                        cout << "Block " << i+1 << ": ";
                        vector<Record> records = accessedBlocks[i]->getRecords();
                        for (int j=0; j<records.size(); j++) {
                            cout << records[j].getTconst() << " ";
                        }
                        cout << "\n";
                        if (i == numBlocks-1) {
                            break;
                        }
                    }
                    

                    // getting average of average ratings from records retrieved
                    float sumOfAverageRatings = 0;
                    int recordCount = 0;
                    for (int i=0; i<accessedBlocks.size()-1; i++) {
                        vector<Record> records = accessedBlocks[i]->getRecords();
                        for (int j=0; j<records.size(); j++) {
                            if (records[j].getNumVotes() >= lowerBound && records[j].getNumVotes() <= upperBound) {
                                sumOfAverageRatings += records[j].getAverageRating();
                                recordCount++;
                            }
                        }
                    }
                    float averageOfAverageRatings = sumOfAverageRatings/recordCount;
                    cout << endl << "Average of averageRatings for movies with numVotes between " << lowerBound << " and " << upperBound << " is " << averageOfAverageRatings << endl;
                }      
            }     
        }

        void updateParentOfLeafNodeAfterDeletion(NonLeafNode* parent, Node* child) {
            for (int i=0; i < parent->nodeSize; i++) {
                    if (parent->nodePointerArr[i] == child && i>0) {
                        parent->keyArr[i-1] = child->keyArr[0];
                    }
                }
        }

        int removeKeyFromTree(int value) {
            cout << "Searching for value: " << value << " within tree.." << endl;
            int numTreeNodesAtStart = numNodes;
            int nodesDeletedCount;
            bool keyFound = false;
            if (rootNode == NULL) {
                cout << "Tree is empty!" << endl;
            }
            pair<LeafNode*, NonLeafNode*> targetLeafParentPair = locateTargetLeafNode(value);
            LeafNode* targetLeafNode = targetLeafParentPair.first;
            NonLeafNode* parentOfTarget = targetLeafParentPair.second;

            for (int i=0; i<targetLeafNode->nodeSize; i++) {
                if (value == targetLeafNode->keyArr[i]) {
                    cout << "Value: " << value << " found within tree! Proceeding to delete." << endl;
                    for (int j=i; j<targetLeafNode->nodeSize-1; j++) {
                        targetLeafNode->keyArr[j] = targetLeafNode->keyArr[j+1];
                        targetLeafNode->blockPointerArr[j] = targetLeafNode->blockPointerArr[j+1];
                    }
                    targetLeafNode->blockPointerArr[NODE_N-1].clear();
                    targetLeafNode->nodeSize--;
                    keyFound = true;
                }
            }
            if (!keyFound) {
                cout << "Key " << value << " not found.";
            } else if (targetLeafNode == rootNode) {
                cout << "Key successfully deleted." << endl;
            } else if (targetLeafNode->nodeSize >= (NODE_N+1)/2) {
                // no need to merge, update parent's key if applicable
                updateParentOfLeafNodeAfterDeletion(parentOfTarget, targetLeafNode);
                cout << "Key successfully deleted." << endl;
            } else {
                // taking keys from sibling or merging required
                int indexLeftSibling, indexRightSibling;
                for (int i=0; i < parentOfTarget->nodeSize; i++) {
                    if (parentOfTarget->nodePointerArr[i] == targetLeafNode) {
                        indexLeftSibling = i-1;
                        indexRightSibling = i+1;
                    }
                }
                // check to see if we can take a key from left sibling
                if (indexLeftSibling >= 0 && parentOfTarget->nodePointerArr[indexLeftSibling]->nodeSize >= (NODE_N+1)/2+1) {
                    LeafNode* leftSibling = static_cast<LeafNode*>(parentOfTarget->nodePointerArr[indexLeftSibling]);
                    for (int i=targetLeafNode->nodeSize; i>0; i--) {
                        targetLeafNode->keyArr[i] = targetLeafNode->keyArr[i-1];
                        targetLeafNode->blockPointerArr[i] = targetLeafNode->blockPointerArr[i-1];
                    }
                    targetLeafNode->keyArr[0] = leftSibling->keyArr[leftSibling->nodeSize-1];
                    targetLeafNode->blockPointerArr[0] = leftSibling->blockPointerArr[leftSibling->nodeSize-1];
                    
                    targetLeafNode->nodeSize++;
                    leftSibling->nodeSize--;
                    updateParentOfLeafNodeAfterDeletion(parentOfTarget, targetLeafNode);
                    updateParentOfLeafNodeAfterDeletion(parentOfTarget, leftSibling);
                    cout << "Key successfully deleted." << endl;

                // check to see if we can take a key from right sibling
                } else if (indexRightSibling <= parentOfTarget->nodeSize && parentOfTarget->nodePointerArr[indexRightSibling]->nodeSize >= (NODE_N+1)/2+1) {
                    LeafNode* rightSibling = static_cast<LeafNode*>(parentOfTarget->nodePointerArr[indexRightSibling]);
                    targetLeafNode->keyArr[targetLeafNode->nodeSize] = rightSibling->keyArr[0];
                    targetLeafNode->blockPointerArr[targetLeafNode->nodeSize] = rightSibling->blockPointerArr[0];

                    for (int i=0; i<rightSibling->nodeSize-1; i++) {
                        rightSibling->keyArr[i] = rightSibling->keyArr[i+1];
                        rightSibling->blockPointerArr[i] = rightSibling->blockPointerArr[i+1];
                    }
                    targetLeafNode->nodeSize++;
                    rightSibling->nodeSize--;
                    updateParentOfLeafNodeAfterDeletion(parentOfTarget, targetLeafNode);
                    updateParentOfLeafNodeAfterDeletion(parentOfTarget, rightSibling);
                    cout << "Key successfully deleted." << endl;
                } else {
                    // have to merge with either right or left sibling
                    if (indexRightSibling <= parentOfTarget->nodeSize) {
                        LeafNode* rightSibling = static_cast<LeafNode*>(parentOfTarget->nodePointerArr[indexRightSibling]);
                        for (int i=targetLeafNode->nodeSize, j=0; j<rightSibling->nodeSize; i++, j++) {
                            targetLeafNode->keyArr[i] = rightSibling->keyArr[j];
                            targetLeafNode->blockPointerArr[i] = rightSibling->blockPointerArr[j];
                        }
                        targetLeafNode->siblingNodePtr = rightSibling->siblingNodePtr;
                        targetLeafNode->nodeSize += rightSibling->nodeSize;
                        // remove pointer in parent

                        delete rightSibling;
                        numNodes--;
                    } else if (indexLeftSibling > 0) {
                        LeafNode* leftSibling = static_cast<LeafNode*>(parentOfTarget->nodePointerArr[indexLeftSibling]);
                        for (int i=leftSibling->nodeSize, j=0; j<targetLeafNode->nodeSize; i++, j++) {
                            leftSibling->keyArr[i] = targetLeafNode->keyArr[i];
                            leftSibling->blockPointerArr[i] = targetLeafNode->blockPointerArr[i];
                        }
                        leftSibling->siblingNodePtr = targetLeafNode->siblingNodePtr;
                        leftSibling->nodeSize += targetLeafNode->nodeSize;
                        // remove pointer in parent

                        delete targetLeafNode;
                        numNodes--;
                    }
                    cout << "Key successfully deleted." << endl;
                    return nodesDeletedCount;
                }
            }
            nodesDeletedCount = numTreeNodesAtStart - numNodes;
            return nodesDeletedCount;
        }
        void removeFromParent(NonLeafNode* parent, Node* childToDelete) {
            if (parent == NULL) {
                return;
            }
            if (parent == rootNode) {
                if (parent->nodeSize == 1) {
                    // change root to child not being deleted
                    if (parent->nodePointerArr[0] == childToDelete) {
                        rootNode = parent->nodePointerArr[1];
                    }
                    else if (parent->nodePointerArr[1] == childToDelete) {
                        rootNode = parent->nodePointerArr[0];
                    }
                    numNodes--;
                    height--;
                    delete parent;
                    return;
                }
            }
            // delete key pointer pair from parent
            int i = 0;
            while(parent->nodePointerArr[i] != childToDelete) {
                i++;
            }
            // i is the index of child pointer to delete
            for (int j=i; j<parent->nodeSize; j++) {
                parent->nodePointerArr[j] = parent->nodePointerArr[j+1];
                parent->keyArr[j-1] = parent->keyArr[j];
            }
            parent->nodeSize--;
            if (parent == rootNode || parent->nodeSize >= NODE_N/2) {
                return;
            }

            NonLeafNode* parentOfParent = parent->parent;
            int indexLeftSibling, indexRightSibling;
            for (int i=0; i<parentOfParent->nodeSize+1; i++) {
                if (parentOfParent->nodePointerArr[i] == parent) {
                    indexLeftSibling = i-1;
                    indexRightSibling = i+1;
                    break;
                }
            }
            // since non leaf nodes need at least floor(n/2) keys, we check is siblings have at least (n/2)+1 keys
            if (indexLeftSibling >= 0 && parentOfParent->nodePointerArr[indexLeftSibling]->nodeSize >= (NODE_N)/2 + 1) {
                NonLeafNode* leftSibling = static_cast<NonLeafNode*>(parentOfParent->nodePointerArr[indexLeftSibling]);
                for (int i=parent->nodeSize; i>0; i--) {
                    parent->keyArr[i] = parent->keyArr[i-1];
                }
                // each key value at i should be the smallest key in the (i+1) subtree
                parent->keyArr[0] = parentOfParent->keyArr[indexLeftSibling];
                parentOfParent->keyArr[indexLeftSibling] = leftSibling->keyArr[leftSibling->nodeSize-1];
                
                for (int i=parent->nodeSize+1; i>0; i--) {
                    parent->nodePointerArr[i] = parent->nodePointerArr[i-1];
                }
                parent->nodePointerArr[0] = leftSibling->nodePointerArr[leftSibling->nodeSize];

                parent->nodeSize++;
                leftSibling->nodeSize--;

            } else if (indexRightSibling <= parentOfParent->nodeSize && parentOfParent->nodePointerArr[indexRightSibling]->nodeSize >= (NODE_N)/2 + 1) {
                NonLeafNode* rightSibling = static_cast<NonLeafNode*>(parentOfParent->nodePointerArr[indexRightSibling]);
                parent->keyArr[parent->nodeSize] = parentOfParent->keyArr[indexRightSibling-1];
                parentOfParent->keyArr[indexRightSibling-1] = rightSibling->keyArr[0];
                parent->nodePointerArr[parent->nodeSize+1] = rightSibling->nodePointerArr[0];

                for (int i=0; i < rightSibling->nodeSize-1; i++) {
                    rightSibling->keyArr[i] = rightSibling->keyArr[i+1];
                }
                for (int i=0; i < rightSibling->nodeSize; i++) {
                    rightSibling->nodePointerArr[i] = rightSibling->nodePointerArr[i+1];
                }

                parent->nodeSize++;
                rightSibling->nodeSize--;
            } else {
                // merge nodes
                if (indexRightSibling <= parentOfParent->nodeSize) {       
                    NonLeafNode* rightSibling = static_cast<NonLeafNode*>(parentOfParent->nodePointerArr[indexRightSibling]);
                    parent->keyArr[parent->nodeSize] = parentOfParent->keyArr[indexRightSibling-1];
                    for (int i = parent->nodeSize+1, j=0; j < rightSibling->nodeSize; i++, j++) {
                        parent->keyArr[i] = rightSibling->keyArr[i];
                    }
                    for (int i = parent->nodeSize+1, j=0; j < rightSibling->nodeSize+1; i++, j++) {
                        parent->nodePointerArr[i] = rightSibling->nodePointerArr[j];
                    }

                    parent->nodeSize += rightSibling->nodeSize + 1;
                    removeFromParent(parentOfParent, rightSibling);
                    numNodes--;
                    delete rightSibling;

                } else if (indexLeftSibling >= 0) {
                    NonLeafNode* leftSibling = static_cast<NonLeafNode*>(parentOfParent->nodePointerArr[indexLeftSibling]);
                    leftSibling->keyArr[leftSibling->nodeSize] = parentOfParent->keyArr[indexLeftSibling];
                    for (int i = leftSibling->nodeSize+1, j=0; j < parent->nodeSize; i++, j++) {
                        leftSibling->keyArr[i] = parent->keyArr[j];
                    }                
                    for (int i = leftSibling->nodeSize+1, j=0; j < parent->nodeSize+1; i++, j++) {
                        leftSibling->nodePointerArr[i] = parent->nodePointerArr[j];
                    }

                    leftSibling->nodeSize += parent->nodeSize + 1;
                    removeFromParent(parentOfParent, parent);
                    numNodes--;
                    delete parent;
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
        } catch (...) {
            continue;
        }

	}
    cout << "Storage of data complete!" << endl;
    cout << "\n--EXPERIMENT 1--" << endl;
    cout << "Number of blocks: " << storage.size() << endl;
    float databaseSize = float(storage.size() * BLOCK_SIZE) / float(1000000);
    cout << "Size of database: " << databaseSize << "MB" << endl;
    cout << "\n--EXPERIMENT 2--" << endl;
    cout << "Beginning to build B+ tree on numVotes.." << endl;
    BPlusTree* bPlusTree = new BPlusTree();
    for (int i=0; i<storage.size(); i++) {
        Block* blockToIndex = &(storage[i]);
        for (int j=0; j<blockToIndex->getRecords().size(); j++) {
            Record recordToIndex = storage[i].getRecords()[j];
            // cout << "inserting block id: " << blockToIndex.getBlockId() << " with index on numVotes: " << recordToIndex.getNumVotes() << " from record: " << recordToIndex.getTconst() << endl;
            bPlusTree->insertIntoTree(recordToIndex.getNumVotes(), blockToIndex);
        }
    }
    cout << "Parameter n: " << NODE_N << endl;
    cout << "Number of nodes in B+ tree: " << bPlusTree->getNumNodes() << endl;
    cout << "Height of B+ tree: " << bPlusTree->getHeight() << endl;
    bPlusTree->printRootNode();
    bPlusTree->printRootFirstChild();
    cout << "\n--EXPERIMENT 3--" << endl;
    bPlusTree->searchValue(500);
    cout << "\n--EXPERIMENT 4--" << endl;
    bPlusTree->searchValueRangeInclusive(30000, 40000);
    cout << "\n--EXPERIMENT 5--" << endl;
    int numNodesDeleted = bPlusTree->removeKeyFromTree(1000);
    cout << "Number of times a node was deleted: " << numNodesDeleted << endl;
    cout << "Number of nodes in updated B+ tree: " << bPlusTree->getNumNodes() << endl;
    cout << "Height of updated B+ tree: " << bPlusTree->getHeight() << endl;
    cout << "Updated "; 
    bPlusTree->printRootNode();
    cout << "Updated ";
    bPlusTree->printRootFirstChild();

    delete newBlockPtr;
}




