#include<stdlib.h>
#include<iostream>
class Logic
{

	private:
		char *green, *red, *blue, *yellow, *white, *orange;

		//randomization parameters
		int rndMin, rndRange;
	public:
		Logic();

		//turning
		void turnX(bool top);		//Green, Red, Blue, Orange
		void turnY(bool left);		//White, Red, Yellow, Orange
		void turnZ(bool inner);		//Green, White, Blue, Yellow
		void turnSelf(char c);		//rotate Side clockwise 90dg
		//reverse turning(3xturn)
		void turnXr(bool top);
		void turnYr(bool left);
		void turnZr(bool inner);	


		void rndCube();
		void clearCube();

		void check();				//Check if all middletiles have right color.

		char* getFace(char c);

		void printField(char c);	//Print specified Field
		void printAll();			//Print all Fields in crossshape
};
