
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <stdexcept>
#include <cmath>
#include "Sudoku.h"

using namespace std;

Sudoku::Sudoku()
{
	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j < 9; j++)
		{
			puz[i][j] = 0;
		}
	}
}



void Sudoku::loadFromFile (string filename)
{
	
		ifstream puzzle_file2(filename, ifstream::in);
		int data, r, c;
		int counter = 0;
		int max = 9;
		while (puzzle_file2 >> data) 
		{
			c = counter%9;
			r = counter/9;
			puz[r][c]= data;
			counter++;
		}
}
	
	
	




bool Sudoku::solve()
{
    int row, col;

	if (!SpotLeft(row, col))
	{
       return true;
	}
    for (int num = 1; num <= 9; num++)
    {
        if (isLegal(row, col, num))
        {
            puz[row][col] = num;
            if (solve())
			{
                return true;
			}
            puz[row][col] = 0;
        }
    }
    return false;
}

// checks if the assignment k to [i][j] is legal or not
bool Sudoku::isLegal( int i, int j, int num)
{
    return row(i, num) && col( j, num) && block(i - i % 3 , j - j % 3, num);
}

// checks if there is a 0 left in the puzzle to be solved
bool Sudoku::SpotLeft(int &row, int &col)
{
    for (row = 0; row < 9; row++)
	{
        for (col = 0; col < 9; col++)
		{
            if (puz[row][col] == 0)
			{
                return true;
			}
		}
	}
    return false;
}


// See if the value k can be in the row
bool Sudoku::row(int i, int k)
{
	for (int j = 0; j < 9; j++)
	{
		if (puz[i][j] == k)
		{
			return false;
		}
	}
	return true;
}

// See if the value K can be in the col
bool Sudoku::col(int j, int k)
{
	for (int i = 0; i < 9; i++)
	{
		if (puz[i][j] == k)
		{
			return false;
		}
	}
	return true;
}

// See if the value K can be in the block
bool Sudoku::block(int i, int j, int k)
{
	for (int row = 0; row < 3; row++)
	{
		for (int col = 0; col <3; col++)
		{
			if (puz[row+i][col+j] == k)
			{
				return false;
			}
		}
	}
	return true;

}





void Sudoku::print() const
{
	for (int i = 0; i < 9; i++)
	{
		if (i == 3 || i == 6)
		{
			cout << "------+-------+------" << endl;
		}
		for (int j = 0; j < 9; j++)
		{
			if (j == 3 || j == 6)
			{
				cout << "| ";
			}
			cout << puz[i][j] << " ";
		}
		cout << endl;


	}

}

bool Sudoku::equals(const Sudoku &other) const
{
	for (int i = 0; i < 9; i++)
	{
		for (int j = 0; j<9; j++)
		{
			if (puz[i][j] != other.puz[i][j])
			{
				return false;
			}

		}

	}
	return true;


}