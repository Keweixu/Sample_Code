
#include <iostream>
#include <string>
using namespace std;

class Sudoku
{
private:
	int puz[9][9];
	bool SpotLeft(int &i, int &j);
	bool row(int i, int k);
	bool col(int j, int k);
	bool block(int i, int j, int k);
	bool isLegal(int i, int j, int k);


public:
	Sudoku();
	void loadFromFile (string filename);
	bool solve();
	void print() const;
	bool equals(const Sudoku &other) const;


	
};