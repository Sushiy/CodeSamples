#include <stdlib.h>
#include <iostream>
#include <cassert>
#include <math.h>
#include <time.h>

#include "logic.h"


Logic::Logic()
{
	green = new char[9];
	blue = new char[9];
	red = new char[9];
	yellow = new char[9];
	white = new char[9];
	orange = new char[9];

	rndMin = 1;
	rndRange = 3;

	clearCube();
}

//TURNMETHODS
void Logic::turnX(bool top)	//Green, Red, Blue, Orange
{
	//std::cout<<"turnX:"<<(top?"top":"bottom")<<"\n";
	char* buffer = new char[3];
	if(top)
	{
		buffer[0] = orange[2];
		buffer[1] = orange[1];
		buffer[2] = orange[0];

		orange[0] = blue[0];
		orange[1] = blue[1];
		orange[2] = blue[2];

		blue[0] = red[2];
		blue[1] = red[1];
		blue[2] = red[0];

		red[0] = green[0];
		red[1] = green[1];
		red[2] = green[2];
		
		green[0] = buffer[0];
		green[1] = buffer[1];
		green[2] = buffer[2];

		turnSelf('y');
	}
	else
	{
		buffer[0] = orange[8];
		buffer[1] = orange[7];
		buffer[2] = orange[6];

		orange[6] = blue[6];
		orange[7] = blue[7];
		orange[8] = blue[8];

		blue[6] = red[8];
		blue[7] = red[7];
		blue[8] = red[6];

		red[6] = green[6];
		red[7] = green[7];
		red[8] = green[8];
		
		green[6] = buffer[0];
		green[7] = buffer[1];
		green[8] = buffer[2];

		turnSelf('w');

	}
	check();
}

void Logic::turnY(bool left)	//White, Red, Yellow, Orange
{
	//std::cout<<"turnY:"<<(left?"left":"right")<<"\n";
	char* buffer = new char[3];
	if(left)
	{
		buffer[0] = orange[6];
		buffer[1] = orange[3];
		buffer[2] = orange[0];

		
		orange[0] = white[0];
		orange[3] = white[3];
		orange[6] = white[6];

		white[0] = red[6];
		white[3] = red[3];
		white[6] = red[0];


		for(int i = 0; i<=6; i+=3)
		{
			red[i] = yellow[i];
			yellow[i] = buffer[i/3];
		}

		turnSelf('b');
		turnSelf('b');
		turnSelf('b');
	}
	else
	{

		buffer[0] = orange[8];
		buffer[1] = orange[5];
		buffer[2] = orange[2];

		orange[8] = white[8];
		orange[5] = white[5];
		orange[2] = white[2];

		white[8] = red[2];
		white[5] = red[5];
		white[2] = red[8];

		for(int i = 2; i<=8; i+=3)
		{
			red[i] = yellow[i];
			yellow[i] = buffer[(i-2)/3];
		}

		turnSelf('g');
		turnSelf('g');
		turnSelf('g');
	}
	check();
}

void Logic::turnZ(bool inner)	//Green, White, Blue, Yellow
{
	//std::cout<<"turnZ:"<<(inner?"inner":"outer")<<"\n";
	char* buffer = new char[3];
	if(inner)
	{
		buffer[0] = green[0];
		buffer[1] = green[3];
		buffer[2] = green[6];

		green[0] = yellow[6];
		green[3] = yellow[7];
		green[6] = yellow[8];

		yellow[6] = blue[6];
		yellow[7] = blue[3];
		yellow[8] = blue[0];
		
		blue[6] = white[8];
		blue[3] = white[7];
		blue[0] = white[6];

		white[8] = buffer[0];
		white[7] = buffer[1];
		white[6] = buffer[2];

		turnSelf('r');
	}
	else
	{
		buffer[0] = green[2];
		buffer[1] = green[5];
		buffer[2] = green[8];

		green[8] = yellow[2];
		green[5] = yellow[1];
		green[2] = yellow[0];

		yellow[2] = blue[2];
		yellow[1] = blue[5];
		yellow[0] = blue[8];
		
		blue[2] = white[0];
		blue[5] = white[1];
		blue[8] = white[2];

		white[0] = buffer[2];
		white[1] = buffer[1];
		white[2] = buffer[0];
	
		turnSelf('o');
	}
	check();
}

void Logic::turnSelf(char c) //Clockwise rotation
{
	char* tmp;
	char buffer0, buffer1;
	switch(c)
	{
		case 'g':
			tmp = green;
			break;
	
		case 'r':
			tmp = red;
			break;

		case 'b':
			tmp = blue;
			break;

		case 'y':
			tmp = yellow;
			break;

		case 'w':
			tmp = white;
			break;

		case 'o':
			tmp = orange;
			break;
	}

	buffer0 = tmp[3];
	buffer1 = tmp[6];
	tmp[3] = tmp[7];
	tmp[6] = tmp[8];
	tmp[7] = tmp[5];
	tmp[8] = tmp[2];
	tmp[5] = tmp[1];
	tmp[2] = tmp[0];
	tmp[1] = buffer0;
	tmp[0] = buffer1;

	check();
}

//REVERSE TURNMETHODS
void Logic::turnXr(bool top)
{
	turnX(top);
	turnX(top);
	turnX(top);
}

void Logic::turnYr(bool left)
{
	turnY(left);
	turnY(left);
	turnY(left);
}

void Logic::turnZr(bool inner)
{
	turnZ(inner);
	turnZ(inner);
	turnZ(inner);
}


//SETCUBE METHODS

void Logic::clearCube()
{
	for(int i = 0; i < 9; ++i)
	{
		green[i] = 'g';
		red[i] = 'r';
		blue[i] = 'b';
		yellow[i] = 'y';
		white[i] = 'w';
		orange[i] = 'o';
	}
}

void Logic::rndCube()
{
	clearCube();

	//determine Number of moves (10-19)

	srand (time(NULL));

	int moves = (rand() % rndRange + rndMin);
	std::cout<< "NumMoves: " << moves << "\n";

	for(int i = 0; i < moves; ++i)
	{
		int key = rand() % 99;
		std::cout<<"key" << i << ": " << key << " "; 
		if(key < 33)
		{
			if(key %2 == 0)
				turnX(true);
			else
				turnX(false);
		}
		else if(key < 66 || key >= 33)
		{
			if(key %2 == 0)
				turnY(true);
			else
				turnY(false);
		}
		else if(key >= 66)
		{
			if(key %2 == 0)
				turnZ(true);
			else
				turnZ(false);
		}
	}
	std::cout<<"\n\n";

}


//ADDITIONAL METHODS
void Logic::check()
{
	assert(green[4] == 'g' && red[4] == 'r' && blue[4] == 'b' && yellow[4] == 'y' && white[4] == 'w'	&& orange[4] == 'o');
}

char* Logic::getFace(char c)
{
	switch(c)
	{
		case 'g':
			return green;
		
		case 'r':
			return red;
		
		case 'b':
			return blue;
		
		case 'y':
			return yellow;
		
		case 'w':
			return white;

		case 'o':
			return orange;

		default:
			std::cout<<"not a valid colorcode \n";

	}
}


void Logic::printField(char c)
{
	switch(c)
	{
		case 'g':
			std::cout<< " | ";
			for(int i = 0; i < 9; ++i)
			{
				std::cout<<green[i]<< " | ";
				if(i == 2 | i == 5)
					std::cout<< "\n | ";
			}
			std::cout<< "\n \n";
			break;
		
		case 'r':
			std::cout<< " | ";
			for(int i = 0; i < 9; ++i)
			{
				std::cout<<red[i]<< " | ";
				if(i == 2 | i == 5)
					std::cout<< "\n | ";
			}
			std::cout<< "\n \n";
			break;
		
		case 'b':
			std::cout<< " | ";
			for(int i = 0; i < 9; ++i)
			{
				std::cout<<blue[i]<< " | ";
				if(i == 2 | i == 5)
					std::cout<< "\n | ";
			}
			std::cout<< "\n \n";
			break;
		
		case 'y':
			std::cout<< " | ";
			for(int i = 0; i < 9; ++i)
			{
				std::cout<<yellow[i]<< " | ";
				if(i == 2 | i == 5)
					std::cout<< "\n | ";
			}
			std::cout<< "\n \n";
			break;
		
		case 'w':
			std::cout<< " | ";
			for(int i = 0; i < 9; ++i)
			{
				std::cout<<white[i]<< " | ";
				if(i == 2 | i == 5)
					std::cout<< "\n | ";
			}
			std::cout<< "\n \n";
			break;

		case 'o':
			std::cout<< " | ";
			for(int i = 0; i < 9; ++i)
			{
				std::cout<<orange[i]<< " | ";
				if(i == 2 | i == 5 | i == 8)
					std::cout<< "\n | ";
			}
			std::cout<< "\n\n";
			break;

		default:
			std::cout<<"not a valid colorcode \n";
	}	
}

void Logic::printAll()
{
	/*
	        * * *
	1       * w *
	        * * *
	  * * * * * * * * * * * *
	2 * g * * r * * b * * o *
	  * * * * * * * * * * * *
	        * * *
	3       * y *
	        * * *
	 */
	//first row
	std::cout<< "      " << " " << white[0] << " " << white[1] << " " << white[2] << " \n";
	std::cout<< "      " << " " << white[3] << " " << white[4] << " " << white[5] << " \n";
	std::cout<< "      " << " " << white[6] << " " << white[7] << " " << white[8] << " \n";
	//secondrow
	std::cout<< " " << green[0] << " " << green[1] << " " << green[2] << " "<< red[0] << " " << red[1] << " " << red[2] << " "<< blue[0] << " " << blue[1] << " " << blue[2] << " "<< orange[0] << " " << orange[1] << " " << orange[2]<< " \n";
	std::cout<< " " << green[3] << " " << green[4] << " " << green[5] << " "<< red[3] << " " << red[4] << " " << red[5] << " "<< blue[3] << " " << blue[4] << " " << blue[5] << " "<< orange[3] << " " << orange[4] << " " << orange[5]<< " \n";
	std::cout<< " " << green[6] << " " << green[7] << " " << green[8] << " "<< red[6] << " " << red[7] << " " << red[8] << " "<< blue[6] << " " << blue[7] << " " << blue[8] << " "<< orange[6] << " " << orange[7] << " " << orange[8]<< " \n";
	//third row
	std::cout<< "      " << " " << yellow[0] << " " << yellow[1] << " " << yellow[2] << " \n";
	std::cout<< "      " << " " << yellow[3] << " " << yellow[4] << " " << yellow[5] << " \n";
	std::cout<< "      " << " " << yellow[6] << " " << yellow[7] << " " << yellow[8] << " \n";

	std::cout<< "\n\n";
}

