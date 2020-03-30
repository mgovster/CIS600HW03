/*
Code By: Matt Govia
SUID: 644471631
CIS600 HW03

final comments:
For the most part compared to HW02 it works a lot better,
I saw the wakeup section for part and product worker only got hit once out of the many times I ran it

It does switch correctly from part and product worker and update everything else correctly it only does
not seem to hit the wakeup notify section much

Possible reason:
the totalWaitTime could not have been carrying over for the next one (possibly not)

In product worker, I didn't notify correctly a part worker and vice verse for the other way
	this could be more likely than the above 

In the wait_until, it would check the predicate at the end, It could be that it checks if there is space 
in the buffer, but since a new part worker is going and product, usually there is always space 
	I can see it being more this problem, or in other words of how I explained it, the predicate is wrong

*/

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <fstream>
//fix the accumulated wait time to zero out at the start of each iteration

using namespace std;

mutex mtx, mtx2;
condition_variable cv;
bool ready{ false };
bool spaceInBuffer{ false };
int Seed = time(NULL);
int buffer[4];
int total_Completed_Packages = 0;
int arraySize = sizeof(buffer) / sizeof(int);
ofstream file;
string fileName{ "log.txt" };

//time point
chrono::steady_clock::time_point programStart;
//if initialized up here, it becomes negative
//when initialized in main it's closest to 0

//I could try to get the very first worker time and subtract it from itself to start at 0
	//problem with this, I don't know how to make sure it's the very first worker being timed


void PartWorker(int id) {
	unique_lock<mutex> lck(mtx);
	ready = false;
	//bool partWorkerDone = false;
	int iterationNum = 1;
	auto firstWorkerTime = chrono::steady_clock::now();
	chrono::microseconds totalWaitTime(0);
	//totalWaitTime is going to be the time it takes the part worker to create the parts, and to 
	//move the parts to the buffer


	//when implementing the wait_until command to see if
	//worker has surpassed the time limit and the predicate is ready, if this is false,
	//then it got woken up by someone, and will change status to wake-up notified.
	//
	//Current worker time computed as
	//Of that current worker minus the previous one (might need a temp time to remember last one)
	//totalWaitTime = duration<int> (workerTime - firstWorker)
	//where workerTime is new steady clock at the start of the loop
	//currtime is the worker's start time

	int assemblyParts[4]{ 0,0,0,0 };
	srand(Seed++);
	int partNum, temp;
	int bufferLimit = 6;

	//zero out Buffer
	for (int i = 0; i < arraySize; i++) { buffer[i] = 0; }

	//now add to the buffer
	//max a buffer can have is 6 5 4 3 respectively

	for (int i = 0; i < 4; i++) {
		temp = rand() % (bufferLimit + 1);
		buffer[i] = temp; // 0 to bufferLimit inclusively
		bufferLimit--;
	}
	//end of adding to Buffer
	//now time for part worker to add their parts in until either a time of 


	//in wait_until
	//1st arg: try to acquire the lock
	//2nd arg: use currWorkerTime + 3000us, make this set to a variable before the waituntil is called
	//becuase in if statement, use to check if this time is reached, if it is end iterations
	//3rd arg: set in variable before waitUntil is called -> check if there's space in buffer 

	int randnum;
	auto nextWorkerTime = firstWorkerTime;

	//let's add the time for the part worker making the parts in the buffer
	//Parts times are 50,70,90,110 to make
	//add to totalWaitTime
	int addMe = 50;
	chrono::microseconds sum(0);
	for (int i = 0; i < arraySize; i++) {
		sum += chrono::microseconds(buffer[i] * addMe);
		addMe += 20;
	}
	totalWaitTime += sum;

	/*
		this method checks if there's space to move the assemblyParts into the buffer
	bufferLimit = 6;
	bool spaceInBuffer = false;
	for (int i = 0; i < sizeof(buffer) / 4; i++) {
		if (assemblyParts[i] != 0 && buffer[i] + assemblyParts[i] <= bufferLimit)
			spaceInBuffer = true;
		bufferLimit--;
	}*/

	//this checks if there is room in the buffer to add more
	bufferLimit = 6;
	bool spaceBBY = false;
	for (int i = 0; i < arraySize; i++) {
		if (buffer[i] < bufferLimit) { spaceBBY = true; }
		bufferLimit--;
	}
	spaceInBuffer = spaceBBY;

	//this is the part that's iffy
	//idk if while loop should be outside wait_until or inside
	//if outside: lock will be released and other threads would intrude in and do there stuff,
	//(could cause problems) but it could go back to the other iterations later on
	//
	//if inside: it will go through almost all iterations (or all it can) until no space in buffer
	//therefore there will most likely always be a full buffer for the product workers to take from

	//Ill do the second method since if I own the power plant (or looking for most money generated)
	//this will lead to the most products completed (full buffer means more products to take from)
	//and it is safer

	//totalCompletedPackages is accumulative 
	//package is when a full pickup order is complete

	//wait until it can grab the lock, if 3000 microseconds from now 
	//has led to a timeout, and it has room in buffer

	if (cv.wait_until(lck, chrono::steady_clock::now() + chrono::microseconds(3000),
		[]() {return spaceInBuffer; })) {
		while (iterationNum <= 5 && spaceInBuffer) {
			srand(Seed++);
			if (iterationNum != 1) { nextWorkerTime = chrono::steady_clock::now(); }
			//let's get the current worker time and print the current Time, Part Worker ID,
			//iteration num, status, and accumulated wait time

			chrono::steady_clock::duration elapse = nextWorkerTime - programStart;
			chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);

			chrono::steady_clock::duration accumulatedWait = totalWaitTime;
			chrono::microseconds relapse = chrono::duration_cast<chrono::microseconds> (accumulatedWait);
			file << "Current Time: " << melapse.count() << "us" << "\n";
			file << "Part Worker ID: " << id << "\n";
			file << "Iteration: " << iterationNum << "\n";
			file << "Status: New Load Order" << "\n";
			file << "Accumulated wait time: " << relapse.count() << "\n";

			//Now I have to print out buffer state, and load order (assemblyParts)
			//After this I will do the computations (add as much as I can from the load order
			//to the buffer state, and then print the updated buffer state and updated load order

			//first build an assemblyParts for the worker, let's zero it out first
			for (int i = 0; i < arraySize; i++) { assemblyParts[i] = 0; }

			//max limit is 4 parts
			partNum = 4;
			//1 spot can't have anything, i.e. it can be 3010, 4000, 1120, 0211
			int ignoreSpot = rand() % 4;	//0 - 3
			int pos = 0;
			//this is for when we are on the spot, it will skip over it
			while (partNum > 0) {
				if (pos != ignoreSpot) {
					randnum = rand() % 2; //0-1
					assemblyParts[pos] += randnum;
					partNum -= randnum;
				}
				pos++;
				if (pos >= 4) { pos = 0; }
			}
			//now let's print the buffer and then the assemblyParts to see if it works well
			//buffer
			file << "Buffer State: (";
			for (int i = 0; i < arraySize; i++) {
				if (i != 3) { file << buffer[i] << ","; }
				else { file << buffer[i]; }
			}
			file << ")" << "\n";

			//assemblyParts
			file << "Load Order: (";
			for (int i = 0; i < arraySize; i++) {
				if (i != 3) { file << assemblyParts[i] << ","; }
				else { file << assemblyParts[i]; }
			}
			file << ")" << "\n";

			//gotta also add time it takes to move parts to buffer (starts at 20)
			addMe = 20;
			chrono::microseconds sum2(0);
			//addMe + (i * 10)
	
			int assemblyLimit = 6;
			int newBuffer[4]{ 0,0,0,0 }, newAssemblyLine[4]{ 0,0,0,0 };
			//assembly line and buffer
			int temp, temp2;
			for (int i = 0; i < arraySize; i++) {		//might have to fix later either here or above
				//later on in iterations, assembly line becomes negative
				temp = 0, temp2;
				if (buffer[i] == assemblyLimit) {
					//if this condition is reached, buffer is already full, nothing needed to add
					temp = buffer[i];
					newBuffer[i] = temp;
					newAssemblyLine[i] = assemblyParts[i];
				}
				else if (buffer[i] + assemblyParts[i] > assemblyLimit) {
					//if this condition is reached, some of assemblyparts[i] goes into buffer
					//set another value to old assemblyParts[i], and old - new equals the number moved
					temp2 = assemblyParts[i];
					temp = (buffer[i] + assemblyParts[i]) - assemblyLimit;
					newBuffer[i] = assemblyLimit;
					newAssemblyLine[i] += temp;
					sum2 += chrono::microseconds((temp2 - newAssemblyLine[i]) * (addMe + (i * 10)));
				}
				else { //if this condition is reached, all of assemblyparts[i] goes into buffer,
					sum2 += chrono::microseconds(assemblyParts[i] * (addMe + (i * 10)));
					temp = buffer[i] + assemblyParts[i];
					newBuffer[i] = temp;
					newAssemblyLine[i] = 0;
				}

				assemblyLimit--;
			}

			totalWaitTime += sum2;
			int n = 0;
			for (int i : newBuffer) { buffer[n] = i; n++; }
			n = 0;
			for (int i : newAssemblyLine) { assemblyParts[n] = i; n++; }
			n = 0;

			file << "Updated Buffer State: (";
			for (int e = 0; e < arraySize; e++) {
				if (e != 3) { file << buffer[e] << ","; }
				else { file << buffer[e]; }
			}
			file << ")\n";

			file << "Updated Load Order: (";
			for (int e = 0; e < arraySize; e++) {
				if (e != 3) { file << assemblyParts[e] << ","; }
				else { file << assemblyParts[e]; }
			}
			file << ")\n";
			//done with Part Worker side if there is space

			//check if there's space again
			bufferLimit = 6;
			bool spaceBBY = false;
			for (int i = 0; i < arraySize; i++) {
				if (buffer[i] < bufferLimit) { spaceBBY = true; }
				bufferLimit--;
			}
			spaceInBuffer = spaceBBY;


			std::this_thread::sleep_for(totalWaitTime);
			file << "\n";
			iterationNum++;
		}
		if (!spaceInBuffer) {
			iterationNum += 420;	//this number doesn't matter, any number > 5 is enough
			ready = true;
			cv.notify_one();
			//ready = true;
		}
		else {
			//else if there is space but we can't move on cause iterations are done
			//we move on anyways
			ready = true;
			cv.notify_one();
		}
		

	}
	else {
		//else if it got wake up notified
		//discard all part worker parts
		chrono::steady_clock::duration elapse = nextWorkerTime - programStart;
		chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);

		chrono::steady_clock::duration accumulatedWait = totalWaitTime;
		chrono::microseconds relapse = chrono::duration_cast<chrono::microseconds> (accumulatedWait);
		file << "Current Time: " << melapse.count() << "us" << "\n";
		file << "Part Worker ID: " << id << "\n";
		file << "Iteration: " << iterationNum << "\n";
		file << "Status: Wakeup-Notified" << "\n";
		file << "Accumulated wait time: " << relapse.count() << "\n";

		//20,30,40,50 to discard current parts in hand (assemblyParts)
		addMe = 20;
		sum -= sum;
		for (int i = 0; i < arraySize; i++) {
			sum += chrono::microseconds(assemblyParts[i] * (addMe + (i*10)));
		}
		totalWaitTime += sum;

		//now that parts are discarded, we can continue on and notify someone else
		cv.notify_one();
	}
	this_thread::sleep_for(totalWaitTime);
	file << "\n";


}



void ProductWorker(int id) {
	unique_lock<mutex> lck(mtx);
	while (!ready) cv.wait(lck);
	//once it's ready (i.e. the buffer is ready) then we can do product worker stuff
	
	int iterationNum = 1;
	auto firstWorkerTime = chrono::steady_clock::now();
	chrono::microseconds totalWaitTime(0);

	chrono::steady_clock::duration elapse = firstWorkerTime - programStart;
	chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);


	int iteration = 1;
	int productLine[] = { 0,0,0,0 };
	int partNum;
	int randnum, cc = 0;
	int pos = 0, addMe;
	bool correct;
	int ignoreSpot;
	srand(Seed++);
	
	//let's check if there's space in Buffer so we can run through the iterations
	//this can be defined as having stuff in the buffer that isn't all 0s
	
	bool notAZero = false;
	for (int i = 0; i < arraySize; i++) {
		if (buffer[i] != 0) { notAZero = true; }
	}
	spaceInBuffer = notAZero;

	auto nextWorkerTime = firstWorkerTime;
	chrono::microseconds sum(0);
	//wait until it can grab the lock, if 6000 microseconds from now 
	//has led to a timeout, and it has room in buffer
	if (cv.wait_until(lck, chrono::steady_clock::now() + chrono::microseconds(6000),
		[]() {return ready; })) {
		totalWaitTime -= totalWaitTime;
		while (iterationNum <= 5 && spaceInBuffer) {
			srand(Seed++);
			if (iterationNum != 1) { nextWorkerTime = chrono::steady_clock::now(); }
			//let's get the current worker time and print the current Time, Part Worker ID,
			//iteration num, status, and accumulated wait time

			chrono::steady_clock::duration elapse = nextWorkerTime - programStart;
			chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);

			chrono::steady_clock::duration accumulatedWait = totalWaitTime;
			chrono::microseconds relapse = chrono::duration_cast<chrono::microseconds> (accumulatedWait);
			file << "Current Time: " << melapse.count() << "us" << "\n";
			file << "Product Worker ID: " << id << "\n";
			file << "Iteration: " << iterationNum << "\n";
			file << "Status: New Pickup Order" << "\n";
			file << "Accumulated wait time: " << relapse.count() << "\n";

			//Now I have to print out buffer state, and load order (assemblyParts)
			//After this I will do the computations (add as much as I can from the load order
			//to the buffer state, and then print the updated buffer state and updated load order

			//max limit is 5 parts
			//can't have 1,1,1,2 or 3,2,0,0 (1 HAS to be 0)
			correct = false;
			
			while (!correct) {
				//zero out in here in case it failed the check the first time
				for (int i = 0; i < arraySize; i++) { productLine[i] = 0; }

				partNum = 5;
				//1 spot can't have anything, but 2 or more cant have nothing
				ignoreSpot = rand() % 4;	//0 - 3
				//this is for when we are on the spot, it will skip over it

				pos = 0;
				while (partNum > 0) {
					if (pos != ignoreSpot) {
						randnum = rand() % 2; //0-1
						productLine[pos] += randnum;
						partNum -= randnum;
					}
					pos++;
					if (pos >= 4) { pos = 0; }
				}

				//now check if this is good
				int zeroCount = 0;
				for (int i = 0; i < arraySize; i++) {
					if (productLine[i] == 0) { zeroCount++; }
				}
				if (zeroCount == 1) { correct = true; }
			}
			



			//now let's print the buffer and then the productLine to see if it works well
			//buffer
			file << "Buffer State: (";
			for (int i = 0; i < arraySize; i++) {
				if (i != 3) { file << buffer[i] << ","; }
				else { file << buffer[i]; }
			}
			file << ")" << "\n";

			//productLine
			file << "Load Order: (";
			for (int i = 0; i < arraySize; i++) {
				if (i != 3) { file << productLine[i] << ","; }
				else { file << productLine[i]; }
			}
			file << ")" << "\n";

			//gotta also add time it takes to move parts from buffer (starts at 20)
			addMe = 20;
			//addMe + (i * 10)
			sum -= sum;
			int newBuffer[4]{ 0,0,0,0 }, newProductLine[4]{ 0,0,0,0 };
			//assembly line and buffer
			int temp, temp2;
			for (int i = 0; i < arraySize; i++) {		//might have to fix later either here or above
				//later on in iterations, assembly line becomes negative
				temp = 0;
				if (buffer[i] < productLine[i]) {
					//I removed all from buffer
					sum += chrono::microseconds(buffer[i] * (addMe + (i*10)));
					newBuffer[i] = 0;
					newProductLine[i] = productLine[i] - buffer[i];
				}
				else if (buffer[i] > productLine[i]) {
					//I removed all of the products I can from the buffer
					sum += chrono::microseconds(productLine[i] * (addMe + (i * 10)));
					newBuffer[i] = buffer[i] - productLine[i];
					newProductLine[i] = 0;
				}
				else {
					//else both are the same value, so they cancel out
					sum += chrono::microseconds(buffer[i] * (addMe + (i * 10)));
					newBuffer[i] = 0;
					newProductLine[i] = 0;
				}
			}

			totalWaitTime += sum;
			int n = 0;
			for (int i : newBuffer) { buffer[n] = i; n++; }
			n = 0;
			for (int i : newProductLine) { productLine[n] = i; n++; }

			file << "Updated Buffer State: (";
			for (int e = 0; e < arraySize; e++) {
				if (e != 3) { file << buffer[e] << ","; }
				else { file << buffer[e]; }
			}
			file << ")\n";

			file << "Updated Load Order: (";
			for (int e = 0; e < arraySize; e++) {
				if (e != 3) { file << productLine[e] << ","; }
				else { file << productLine[e]; }
			}
			file << ")\n";
			//let's check if it counts as a completed product
			//this is done when all of the updated Buffer is 0
			cc = 0;
			for (int i = 0; i < arraySize; i++) {
				if (productLine[i] == 0) { cc++; }
			}
			//4 0's found
			if (cc >= 4) { total_Completed_Packages++; }
			file << "Total Completed Packages: " << total_Completed_Packages << "\n";

			//done with Product Worker side if there is space
			



			//check if there's space again
			bool notAZero = false;
			for (int i = 0; i < arraySize; i++) {
				if (buffer[i] != 0) { notAZero = true; }
			}
			spaceInBuffer = notAZero;


			std::this_thread::sleep_for(totalWaitTime);
			file << "\n";
			iterationNum++;
		}
		if (!spaceInBuffer) {	
			//if no more space to take from buffer, end iterations and discard current productLine
			iterationNum += 500;
			sum -= sum;
			addMe = 80;
			for (int i = 0; i < arraySize; i++) {
				sum += chrono::microseconds(productLine[i] * (addMe + (i*20)));
			}
			totalWaitTime += sum;
			//ready = true;
		}
		//cv.notify_all();
	}
	else {
		//else if it was woken up then do stuff
		//if this is hit then a timeout has occured, and discard all parts that have been picked up
		//add this time to totalWaitTime and wait

		chrono::steady_clock::duration elapse = nextWorkerTime - programStart;
		chrono::microseconds melapse = chrono::duration_cast<chrono::microseconds> (elapse);

		chrono::steady_clock::duration accumulatedWait = totalWaitTime;
		chrono::microseconds relapse = chrono::duration_cast<chrono::microseconds> (accumulatedWait);
		file << "Current Time: " << melapse.count() << "us" << "\n";
		file << "Product Worker ID: " << id << "\n";
		file << "Iteration: " << iterationNum << "\n";
		file << "Status: Wakeup-Timeout" << "\n";
		file << "Accumulated wait time: " << relapse.count() << "\n";


		bool correct = false;
		int ignoreSpot, pos;
		while (!correct) {
			//zero out in here in case it failed the check the first time
			for (int i = 0; i < arraySize; i++) { productLine[i] = 0; }

			partNum = 5;
			//1 spot can't have anything, but 2 or more cant have nothing
			ignoreSpot = rand() % 4;	//0 - 3
			//this is for when we are on the spot, it will skip over it

			pos = 0;
			while (partNum > 0) {
				if (pos != ignoreSpot) {
					randnum = rand() % 2; //0-1
					productLine[pos] += randnum;
					partNum -= randnum;
				}
				pos++;
				if (pos >= 4) { pos = 0; }
			}

			//now check if this is good
			int zeroCount = 0;
			for (int i = 0; i < arraySize; i++) {
				if (productLine[i] == 0) { zeroCount++; }
			}
			if (zeroCount == 1) { correct = true; }
		}


		//now let's print the buffer and then the productLine to see if it works well
		//buffer
		file << "Buffer State: (";
		for (int i = 0; i < arraySize; i++) {
			if (i != 3) { file << buffer[i] << ","; }
			else { file << buffer[i]; }
		}
		file << ")" << "\n";

		//productLine
		file << "Load Order: (";
		for (int i = 0; i < arraySize; i++) {
			if (i != 3) { file << productLine[i] << ","; }
			else { file << productLine[i]; }
		}
		file << ")" << "\n";



		//^all of this same as a normal product worker, now I just have to discard (throw away) 
		//the parts that get taken from the buffer get deleted
		
		addMe = 80;
		//addMe + (i * 20)
		//plan: we have the buffer and ProductLine
		//subtract what we can from the buffer and the product line and put into a new array 
		//the amount we taken from the buffer, this is what we can work with

		sum -= sum;
		//zero out sum so we can use it
		int discardPile[4]{ 0,0,0,0 }, newBuffer[4]{ 0,0,0,0 }, newProductLine[4]{ 0,0,0,0 };
		int temp, temp2;
		for (int i = 0; i < arraySize; i++) {
			temp = 0, temp2 = 0;
			if (buffer[i] < productLine[i]) {
				//then all from buffer is getting removed and discarded
				//discardPile[i] is going to be buffer[i]
				discardPile[i] = buffer[i];
				sum += chrono::microseconds(discardPile[i] * (addMe + (i * 20)));
				newBuffer[i] = 0;
				newProductLine[i] = productLine[i] - buffer[i];
			}
			else if (buffer[i] > productLine[i]) {
				//discardPile[i] = productLine[i]
				//subtract buffer[i] - productLine[i]
				//
				discardPile[i] = productLine[i];
				sum += chrono::microseconds(discardPile[i] * (addMe + (i * 20)));
				newBuffer[i] = buffer[i] - productLine[i];
				newProductLine[i] = 0;
			}
			else {
				//buffer[i] = productLine[i]

				discardPile[i] = productLine[i] - buffer[i];
				sum += chrono::microseconds(discardPile[i] * (addMe + (i * 20)));
				newBuffer[i] = 0;
				newProductLine[i] = 0;
			}
		}
		totalWaitTime += sum;
		int n = 0;
		for (int i : newBuffer) { buffer[n] = i; n++; }
		n = 0;
		for (int i : newProductLine) { productLine[n] = i; n++; }
		n = 0;
		

		file << "Updated Buffer State: (";
		for (int e = 0; e < arraySize; e++) {
			if (e != 3) { file << buffer[e] << ","; }
			else { file << buffer[e]; }
		}
		file << ")\n";

		file << "Updated Load Order: (";
		for (int e = 0; e < arraySize; e++) {
			if (e != 3) { file << productLine[e] << ","; }
			else { file << productLine[e]; }
		}
		file << ")\n";
		//check if there's space again
		bool notAZero = false;
		for (int i = 0; i < arraySize; i++) {
			if (buffer[i] != 0) { notAZero = true; }
		}
		spaceInBuffer = notAZero;


		std::this_thread::sleep_for(totalWaitTime);
		file << "\n";
		iterationNum++;
	}
	this_thread::sleep_for(totalWaitTime);

}



int main() {
	auto startT = chrono::steady_clock::now();
	programStart = startT;
	srand(Seed);
	file.open(fileName);


	const int m = 20, n = 16; //m: number of Part Workers
	//n: number of Product Workers
	//m>n
	thread partW[m];
	thread prodW[n];
	for (int i = 0; i < n; i++) {
		partW[i] = thread(PartWorker, i);
		prodW[i] = thread(ProductWorker, i);
	}
	for (int i = n; i < m; i++) {
		partW[i] = thread(PartWorker, i);
	}
	/* Join the threads to the main threads */
	for (int i = 0; i < n; i++) {
		partW[i].join();
		prodW[i].join();
	}
	for (int i = n; i < m; i++) {
		partW[i].join();
	}

	file.close();
	cout << "Finish!" << endl;
	return 0;
}
