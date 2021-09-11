#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <sstream>

using namespace std;

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
		string tconst;
		double averageRating;
		int numVotes;
	public:
		Record(string tconst, double averageRating, int numVotes) {
			this->tconst = tconst;
			this->averageRating = averageRating;
			this->numVotes = numVotes;
		}
};

class Block {
	private:
		vector<Record> records;
	public:
		Block() {
			cout << "New Block created" << endl; 
		}
		void* operator new(size_t size) {
			std::cout << "Overloading new operator with size: 100 instead of size: " << size << std::endl;
			void* p = ::operator new(100);
			return p;
		}
		void addRecord(Record record) {
			records.push_back(record);
		}

};

int main(int argc, char *argv[])
{
	std::cout << "Program begins: " << std::endl;
	// std::ifstream data ("./data.tsv");
	// std::string line;
	// while (std::getline(data, line)) {
	// 	vector<string> row_values;
	// 	row_values = split(line, '\t');
	// 	for (auto v: row_values) {
	// 		cout << v << ',';
	// 	}
	// 	cout << endl;
	// }
	Block* blockPtr = new Block();
	cout << sizeof(*blockPtr) << endl;
	Record record1 = Record("id", 5.0, 500);
	blockPtr->addRecord(record1);
	cout << sizeof(*blockPtr) << endl;
	blockPtr->addRecord(record1);
	blockPtr->addRecord(record1);
	blockPtr->addRecord(record1);
	blockPtr->addRecord(record1);
	blockPtr->addRecord(record1);
	blockPtr->addRecord(record1);
	cout << sizeof(*blockPtr) << endl;
	delete blockPtr;
}



