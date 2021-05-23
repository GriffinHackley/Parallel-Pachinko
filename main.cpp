#include <iostream>
#include <mpi.h>
#include <unistd.h>
#include <stdlib.h>
#include <numeric>

#define MCW MPI_COMM_WORLD

using namespace std;

class Ball {
	public:
		int column;
		int row;
		//row must be an even number
		Ball() {column = 0; row = 4;}
};

int direction(Ball current, int boardSize){
	//if the ball is at the edges then it can only go inwards
	if(current.column > boardSize){
		return -1;
	}
	if(current.column < -1*boardSize){
		return 1;
	}

	
	//if the random number is even go right, if odd go left
	int rank;
	MPI_Comm_rank(MCW, &rank);

	srand(time(NULL)*rank*rand());
	int random = rand()%2;
	if(random == 0){
		return 1;
	} else {
		return -1;
	}
}

//simulate each ball individually
int simulate(Ball &current, int boardSize){
	if(current.row == 0){
		return 0;
	}
	current.row--;
	current.column += direction(current, boardSize);
	simulate(current, boardSize);
	return current.column;
}

void honorableLeader(Ball balls[], int length, int size, int boardSize){
	int score[1+(boardSize*2)];
	fill_n(score, 1+(boardSize*2), 0);
	int done[size];
	int data;
	MPI_Status status;
	int go = 1;

	//if not enough tasks for workers only assign as many as needed
	if(length < size-1){
		cout << "not enough tasks" << endl;
		for(int i = 1; i < length; i++){
			data = i;
			MPI_Send(&data,1,MPI_INT,i,0,MCW);
		}
		for(int i = length; i < size; i++){
			data = -10;
			MPI_Send(&data,1,MPI_INT,i,0,MCW);
			done[i] = 1;
		}
	}

	//give work to all workers
	for(int i = 1; i < size; i++){
		data = i;
		MPI_Send(&data,1,MPI_INT,i,0,MCW);
	}

	//wait for workers to be done and assign more work
	for(int i = size; i<=length-1;  i++){
		MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, 0, MCW,&status);
		score[data+boardSize]++;
		data=i;
		MPI_Send(&data,1,MPI_INT,status.MPI_SOURCE,0,MCW);
	}

	//wait for all workers to be done
	while(go){
		MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, 0, MCW,&status);
		done[status.MPI_SOURCE] = 1;
		score[data+boardSize]++;
		
		//wait until the last one finishes
		go = 0;
		for(int i = 1; i<size;  i++){
			if(done[i] != 1){
				go = 1;
			}
			data=i;
		}
	}

	//send workers home
	data = -10;
	for(int i = 1; i < size; i++){
		MPI_Send(&data,1,MPI_INT,i,0,MCW);
	}

	//print results
	int counter = 0;
	for(int i = 1; i < (sizeof(score)/sizeof(score[0]))-2; i++){
		if(i%2 != 0){
			i++;
		}

		// divide by 2 because if the number of times the ball needs to fall is even then it will always land in an even numbered bucket
		cout << i/2 << ":";

		// comment out to not print number of ball in each bucket
		// cout << score[i];
		
		for(int j = 1; j<=score[i]; j++){
			cout << "x";
			counter++;
		}
		cout << endl;
	}
}

void gloriousWorker(Ball balls[], int rank, int boardSize){
	int data;
	//give work to glorious workers
	while(1){
		//if terminate code, end process
		MPI_Recv(&data, 1, MPI_INT, 0, 0, MCW,MPI_STATUS_IGNORE);
		if(data == -10){
			break;
		}
		int score = simulate(balls[data], boardSize);
		MPI_Send(&score,1,MPI_INT,0,0,MCW);
	}
}

int main(int argc, char **argv){
	//set seed for random function
	srand(time(NULL));   

	//initialize MPI
	int rank, size;
    int data;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MCW, &rank); 
    MPI_Comm_size(MCW, &size);

	//set up game
	int boardSize = 6;
	int length = 100;
	Ball allBalls[length];

	simulate(allBalls[0], boardSize);
	
	//set up honorable leader and glorious workers
	if(rank == 0){
		honorableLeader(allBalls, length, size, boardSize);
	} else {
		gloriousWorker(allBalls, rank, boardSize);
	}

	// cout << allBalls[0].row << endl;

	MPI_Finalize();
    
	return 0;
}