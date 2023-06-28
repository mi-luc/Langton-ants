#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#define BLACK 1 // roteste sens trigonometric
#define WHITE 2 // roteste sens orar
#define EMPTY_SLOT 0

#define LEFT 0
#define RIGHT 2
#define UP 1
#define DOWN 3

#define UNCHANGED 0
#define CHANGED 1

#define ERASED 0
#define NOT_ERASED 1

#define NR_VECINI 10

#define SENTBUFFER 800
typedef struct Furnica
{
	int index;
	int nextMove;
} Ant;

typedef struct LangtonsAnts
{
	Ant antsData[100000];
	int size;
} LantonsAnts;

LantonsAnts ants;
int N;
char *filenameIN, *filenameOUT;
char *map;
char *wasMapUsed;
int width, height;
int *nrTotFurnici;
int antWasErased = NOT_ERASED;
int low;
int high;
int rank;
int nProcesses;
int antsNeighboursVector[10000];
int antsNeighbourSize=0;

int myNeighbours[NR_VECINI];
int sizeVecini = 0;
int sendBuffer[SENTBUFFER];
int recvBuffer[SENTBUFFER];

// ANTS MOVEMENT
void addAntToVector(Ant furnica,int rankToSend)
{
	//RankToSend //index //nextMove
	antsNeighboursVector[3*antsNeighbourSize]=rankToSend;
	antsNeighboursVector[3*antsNeighbourSize+1]=furnica.index;
	antsNeighboursVector[3*antsNeighbourSize+2]=furnica.nextMove;
	antsNeighbourSize++;
}
void indexToCoord(int *x, int *y, int index)
{
	*x = index / width;
	*y = index % width;
}
int negateColor(int color)
{
	if (color == BLACK)
		return WHITE;
	if (color == WHITE)
		return BLACK;
}
int coordToIndex(int x, int y)
{
	return x * width + y;
}
void changeAngle(int *currentAngle, char tileColor)
{
	if (tileColor == BLACK) // trigonometric
	{
		*currentAngle = (*currentAngle - 1 + 4) % 4;
	}
	if (tileColor == WHITE) // orar
	{
		*currentAngle = (*currentAngle + 1 + 4) % 4;
	}
}
void eraseAnt(int index)
{
	if (ants.size > 0)
	{
		antWasErased = ERASED;
		for (int i = index; i < ants.size - 1; i++)
		{
			ants.antsData[i] = ants.antsData[i + 1];
		}
		ants.size--;
	}
}
int getMyRank(int tile)
{
	int ranked=0;
	while(ranked*ceil(width*height/nProcesses) <= tile)
	{
		ranked++;
	}
	return ranked-1;
}
void MoveAnt(int index)
{
	Ant *currentAnt = &(ants.antsData[index]);
	int location = currentAnt->index;
	int direction = currentAnt->nextMove;
	int x, y;
	indexToCoord(&x, &y, location);
	// printf("xxx%i %ixxx\n", x, y);
	switch (direction)
	{
	case UP:
		x--;
		break;
	case DOWN:
		x++;
		break;
	case LEFT:
		y--;
		break;
	case RIGHT:
		y++;
		break;
	default:
		exit(1);
	}
	int newLocation = coordToIndex(x, y);
	if (x < 0 || x >= height || y < 0 || y >= width)
	{
		//BAD MOVE, OUT OF BOUNDS
		eraseAnt(index);
		return;
	}
	else if (newLocation < low || newLocation >= high)
	{
		//READY TO SEND TO NEIGHBOUR
		
		int rankToSend=getMyRank(newLocation);
		// floor((double)(newLocation*nProcesses/(width*height)));
		// if(newLocation==high)
		// 	rankToSend++;
		//printf("I am rank %d sending index:%d to rank %d low:%d high:%d\n",rank,newLocation,rankToSend,low,high);
		ants.antsData[index].index=newLocation;
		
		addAntToVector(ants.antsData[index],rankToSend);
		eraseAnt(index);
		return;
	}
	else
	{
		//GOOD MOVE
		// printf("xxx%i %ixxx\n", x, y);
		ants.antsData[index].index = newLocation;
		return;
	}
}

void getArgs(int argc, char **argv)
{
	if (argc < 3)
	{

		printf("Not enough paramters: ./program N printLevel\nprintLevel: 0=no, 1=verbouse, 2=partial\n");
		exit(1);
	}

	filenameIN = (char *)malloc((strlen(argv[1] )+1) * sizeof(char));
	filenameOUT = (char *)malloc((strlen(argv[2] )+1) * sizeof(char));
	memcpy(filenameIN, argv[1], strlen(argv[1]) + 1);
	memcpy(filenameOUT, argv[2], strlen(argv[2] + 1));
}
void calculateLimits(int rank, int nProcceses)
{
	int size = width * height;
	
	
	low=rank*ceil(size/nProcceses);
	high=fmin(size, (rank + 1) * ceil(size/nProcceses));
	if(rank==nProcceses-1 )
	high=size;
	// low = rank * size / nProcceses;
	// high = (rank + 1) * size / nProcceses;
	//printf("%d %d",low,high);
}
void readFile()
{
	ants.size = 0;
	FILE *fin = fopen(filenameIN, "r");
	if (fin == NULL)
		exit(EXIT_FAILURE);
	char lineBuffer[8000];
	int first = 0;
	int position = 0;
	while (fgets(lineBuffer, 7000, fin) != NULL)
	{
		// printf("%s", lineBuffer);
		if (first == 0)
		{
			char *tokens = strtok(lineBuffer, " \n");
			int i = 0;
			while (tokens != NULL)
			{
				switch (i)
				{
				case 0:
					height = atoi(tokens);
					break;
				case 1:
					width = atoi(tokens);
					break;
				case 2:
					N = atoi(tokens);
					break;
				default:
					break;
				}
				i++;
				tokens = strtok(NULL, " \n");
			}
			map = (char *)malloc(width * height * sizeof(char));
			wasMapUsed = (char *)malloc(width * height * sizeof(char));
			memset(wasMapUsed, UNCHANGED, width * height * sizeof(char));
		}
		calculateLimits(rank, nProcesses);
		if (first == 1)
		{
			char *tokens = strtok(lineBuffer, " \n");
			while (tokens != NULL)
			{
				if (position >= low && position < high)
				{
					int valueOfTile = tokens[0] - '0';
					if (valueOfTile == 0)
					{
						map[position++] = BLACK;
					}
					else if (valueOfTile == 1)
					{
						map[position++] = WHITE;
					}
				}
				else
				{
					map[position++] = EMPTY_SLOT;
				}
				for (int j = 1; j < strlen(tokens); j++)
				{
					ants.antsData[ants.size].index = position - 1;
					ants.antsData[ants.size].nextMove = tokens[j] - '0';
					ants.size++;
				}
				tokens = strtok(NULL, " \n");
			}
		}
		first = 1;
	}
	fclose(fin);
}
void printMap(char *shownMap)
{
	printf("RANK%d %d %d\n", rank,low,high);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			printf("%c ", *(shownMap + i * width + j) + '0' - 1);
		}
		printf("\n");
	}
}
void printAnts(int step)
{
	printf("RANK %d my ants and my map and my step:%d\n\n",rank,step);
	printMap(map);
	for (int i = 0; i < ants.size; i++)
	{
		printf("%d %d, ", ants.antsData[i].index, ants.antsData[i].nextMove);
	}
	printf("xxxxxxx\n");
}
void writeResult()
{
	char lineBuffer[8096];
	FILE *fout = fopen(filenameOUT, "w");
	sprintf(lineBuffer, "%d %d\n", height, width);
	fwrite(lineBuffer, 1, strlen(lineBuffer), fout);
	for (int i = 0; i < height; i++)
	{
		lineBuffer[0] = '\0';
		// WRITE THIS TO BUFFER AND FLUSH OUT IN FILE
	}
	return;
}

int numberToSendAnts(int rankToSendTo)
{
	int size=0;
	for(int i=0;i<antsNeighbourSize;i++)
	{
		if(size*2+1>SENTBUFFER)
			{
				exit(12);
			}
		if(antsNeighboursVector[3*i]==rankToSendTo)
		{
			size++;
		}
	}
	return size;
	//
	//printf("!!!SIZE:%d ON RANK%d\n",size,rank);
}
void sendHisAnts(int v[],int rankToSendTo)
{
	memset(v,-1,SENTBUFFER);
	int size=0;
	for(int i=0;i<antsNeighbourSize;i++)
	{
		if(size*2+1>SENTBUFFER)
			{
				exit(12);
			}
		if(antsNeighboursVector[3*i]==rankToSendTo)
		{
			v[size*2]=antsNeighboursVector[3*i+1];
			v[size*2+1]=antsNeighboursVector[3*i+2];
			size++;
		}
	}
	//
	//printf("!!!SIZE:%d ON RANK%d\n",size,rank);
}
void getMyAnts(int* v,int step)
{
	int i=0;
	//index //nextMove
	while(v[i]!=-1)
	{
		//printf("%d %d\n",rank,i);
		if(i>SENTBUFFER)
		{
			printf("HERE");
			exit(13);
		}
		//printf("I am rank %d, step:%d, received index:%d\n",rank,step,v[i]);
		//printf("ants size:%d rank%d\n",ants.size,rank);
		ants.antsData[ants.size].index=v[i];
		ants.antsData[ants.size].nextMove=v[i+1];
		ants.size++;
		i+=2;
		
	}
}
void processOneMove(int step)
{

	memset(wasMapUsed, UNCHANGED, width * height * sizeof(char));
	antsNeighbourSize=0;
	for (int i = 0; i < ants.size; i++)
	{
		if (wasMapUsed[ants.antsData[i].index] == UNCHANGED)
		{
			if (map[ants.antsData[i].index] == BLACK)
			{
				map[ants.antsData[i].index] = WHITE;
				wasMapUsed[ants.antsData[i].index] = CHANGED;
				continue;;
			}
			else if (map[ants.antsData[i].index] == WHITE)
			{
				map[ants.antsData[i].index] = BLACK;
				wasMapUsed[ants.antsData[i].index] = CHANGED;
				continue;
			}
		}
		
	}
	for (int i = 0; i < ants.size; i++)
	{
		changeAngle(&ants.antsData[i].nextMove, negateColor(map[ants.antsData[i].index]));
		MoveAnt(i);
		if (antWasErased == ERASED)
		{
			antWasErased = NOT_ERASED;
			i--;
		}
	}
	//SEND TO NEIGHBOURS
	//MPI_Barrier(MPI_COMM_WORLD);
	//////OPTIMIZE BUFFERS
	int sizes[64];
	memset(sizes,-1,10);
	for(int i=0;i<sizeVecini;i++)
	{
		sizes[i]=numberToSendAnts(myNeighbours[i]);
		
	}
	for(int i=0;i<sizeVecini;i++)
	{
		//char v[SENTBUFFER];
		
		sendHisAnts(sendBuffer,myNeighbours[i]);

		// for(int i=0;i<50;i++)
		// 	printf("%d ",sendBuffer[i]);
		//printf("\nxxxxxxxxxxx\n");
		//printf("%d->%d\n",rank,myNeighbours[i]);
		
		MPI_Send(sendBuffer,SENTBUFFER,MPI_INT,myNeighbours[i],0,MPI_COMM_WORLD);
		
		//printf("I AM RANK%d SEND!!!\n",rank);
		/*
		for(int i=0;i<SENTBUFFER;i++)
			printf("%d ",recvBuffer[i]);
		printf("\n");*/
	}

	MPI_Barrier(MPI_COMM_WORLD);

	//RECEIVE FROM NEIGHBOURS
	//printf("%d STUCK\n",rank);
	for(int i=0;i<sizeVecini;i++)
	{
		//char v[SENTBUFFER];
		//printf("%d<-%d\n",rank,myNeighbours[i]);
		MPI_Recv(recvBuffer,SENTBUFFER,MPI_INT,myNeighbours[i],0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		//printf("I AM RANK%d RECV!!!\n",rank);
		/*
		for(int i=0;i<SENTBUFFER;i++)
			printf("%d ",recvBuffer[i]);
		printf("\n");*/
		getMyAnts(recvBuffer,step);

	}
    //printf("%d UNSTUCK\n",rank);
	//MPI_Barrier(MPI_COMM_WORLD);
	//printf("RECEIVED BY RANK %d",rank);
	
}
void gameRun()
{

	for (int i = 0; i < N; i++)
	{
		//printf("RANK:%d, step %d size:%d\n",rank,i+1,ants.size);
		//printAnts(i+1);
		processOneMove(i+1);
		//printf("RANK:%d, step %d size:%d index:%d \n",rank,i+1,ants.size,ants.antsData[0].index);
		
		//printMap(map);
	}
}

void restrainAnts()
{
	for (int i = 0; i < ants.size; i++)
	{
		if (ants.antsData[i].index < low || ants.antsData[i].index >= high)
		{
			eraseAnt(i);
			i--;
		}
	}
	//printf("Rank %d, antssize:%d index:%d",rank,ants.size,ants.antsData[0].index);
	antWasErased = NOT_ERASED;
	
}


int isInSet(int vecin)
{
	for (int i = 0; i < NR_VECINI; i++)
	{
		if (vecin == myNeighbours[i])
			return 1;
	}
	return 0;
}
void addToSet(int vecin)
{

	if (sizeVecini == 10)
		exit(10);
	// printf("##rank%d #%d ",rank,vecin);
	if (!isInSet(vecin) && vecin != rank && vecin >= 0 && vecin < nProcesses)
	{

		myNeighbours[sizeVecini] = vecin;
		sizeVecini++;
	}
}

void getNeighbours()
{

	memset(myNeighbours, -1, NR_VECINI * sizeof(int));
	int lenInterval = width * height;
	// printf("$%d %d %d\n\n",low,high,lenInterval);
	if((low-1)/width==low/width)
	addToSet(getMyRank(low-1));
	if((high+1)/width==high/width)
	addToSet(getMyRank(high+1));
	for (int i = low; i < high; i++)
	{

		addToSet(getMyRank(i-width));
		addToSet(getMyRank(i+width));
	}
}
void seeSet()
{
	printf("I am rank %d: ", rank);
	for (int i = 0; i < NR_VECINI; i++)
	{
		printf("%d ", myNeighbours[i]);
	}
	printf("I was rank %d!!!\n\n", rank);
}

void init()
{
	readFile();
	//MPI_Bcast(map,width*height,MPI_CHAR,0,MPI_COMM_WORLD);
	//Read only for rank 0... Send Matrix for the rest ant the ants...
	//Maybe faster?
	//Fix send ants when vector is null-> send vector with variable size -1 NULL x (size)
	//Fix game mechanics... not working -np 4 for in_5.txt
	getNeighbours();
	restrainAnts();
}

//FA AFISAREA PENTRU FURNICI PE HARTA >>>>
//VEZI DE CE NU MERGE PT ANUMITE VALORI >>>


//ANOTHER TIME:
//SEND ONLY WHEN NEEDED ! >>>   !!!!
//READ MAP ONLY ON 0 AND SEND?? > !!!!!
int main(int argc, char *argv[])
{
	getArgs(argc, argv);

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);

	init();
	
	seeSet();
	//printMap(map);
	gameRun();

	
	char *finalMap = malloc(width * height * sizeof(char));
	

	MPI_Reduce(map,finalMap,width*height,MPI_CHAR,MPI_SUM,0,MPI_COMM_WORLD);

	if(rank==0)
	{
		printf("FINAL MAP!!!\n\n\n");
		printMap(finalMap);
	}
	free(map);
	free(finalMap);
	MPI_Finalize();
	return 0;
}
