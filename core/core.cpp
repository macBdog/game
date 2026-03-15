#include <iostream>
#include <math.h>

#include "Vector.h"
#include "LinkedList.h"
#include "Iterator.h"
#include "LinearAllocator.h"


int main(int argc, char * argv)
{
	// This file is intended as a test stub for the core project

	// Generate some vector data to test on
	const static int numTestData = 5;
	Vector vecs[numTestData];
	LinkedListNode<Vector> nodes[numTestData];

	// Insert test data into our list
	LinkedList<Vector> listOfVectors;
	for (int i = 0; i < numTestData; ++i)
	{
		vecs[i] = Vector(float(rand() % 10), float(rand() % 10), float(rand() % 10));
		nodes[i].SetData(&vecs[i]);
		listOfVectors.Insert(&nodes[i]);
	}

	// Walk over list using loops
	LinkedListNode<Vector> * currentNode = listOfVectors.GetHead();
	while (currentNode != 0)
	{
		printf("%.2f, %.2f, %.2f, Length = %f\n", 		currentNode->GetData()->GetX(), 
														currentNode->GetData()->GetY(), 
														currentNode->GetData()->GetZ(), 
														currentNode->GetData()->Length());
		currentNode = currentNode->GetNext();
	}

	// Sort the list
	listOfVectors.SlowSort();
	listOfVectors.MergeSort();
	printf("\n ************************************************************************\n \n");

	// Walk over list using iterator
	Iterator<LinkedListNode<Vector>> iter(listOfVectors.GetHead());
	while (iter.Resolve() != 0)
	{
		printf("%.2f, %.2f, %.2f, Length = %f\n",	iter.Resolve()->GetData()->GetX(), 
													iter.Resolve()->GetData()->GetY(),
													iter.Resolve()->GetData()->GetZ(),
													iter.Resolve()->GetData()->Length());
		iter++;
	}

	return 0;
}