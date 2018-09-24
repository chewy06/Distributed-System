# Distributed-System
 * This program will calculate the shortes path to the Traveling Salesman Problem(TSP). 
 * Using openmpi, the program will create a number of nodes that will generate
 * unique random tours. Based on the random tour new tours will be generated. The tour
 * with the lowest cost will be broadcasted in order to find the tour with the lowest cost.
